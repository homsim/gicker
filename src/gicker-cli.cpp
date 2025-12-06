//
// Created by simon on 06.11.25.
//

#include "gicker-cli.h"

#include <stdexcept>
#include <cstdlib>
#include <cstring>

bool GickerCli::isCommandInstalled(const std::string &command) {
    return std::system(("command -v " + command + " > /dev/null 2>&1").c_str()) == 0;
}

bool GickerCli::isGitInstalled() {
    return isCommandInstalled("git");
}

bool GickerCli::isDockerInstalled() {
    return isCommandInstalled("docker");
}

picojson::value GickerCli::getDockerMetaData(const std::string &id, const DockerElem docker_elem) {
    FILE *pipe = popen(
    ("docker " + toString(docker_elem) + " inspect " + id).c_str(),
    "r");

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    // read to dynamic buffer in 4kb chunks
    std::string result;
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        result.append(buffer, bytes_read);
    }

    if (pclose(pipe) != 0) {
        throw std::runtime_error("Docker command failed - " + toString(docker_elem)
            + " " + id + " may not exist");
    }

    picojson::value json;

    const std::string err = picojson::parse(json, result);

    if (!picojson::parse(json, result).empty()) {
        throw std::runtime_error(err);
    }
    if (!json.is<picojson::value::array>()) {
        throw std::runtime_error("Expected JSON array from docker inspect");
    }

    return json;
}

std::filesystem::path GickerCli::getContainerSourcePath(const picojson::value &json) {
    // Find "Config"."Labels"."com.docker.compose.project.working_dir"

    const auto &arr = json.get<picojson::value::array>();
    if (arr.empty()) {
        throw std::runtime_error("Empty array from docker inspect");
    }

    const auto &container = arr[0];
    if (!container.is<picojson::value::object>()) {
        throw std::runtime_error("Expected container object");
    }

    const auto &container_obj = container.get<picojson::value::object>();
    const auto config_it = container_obj.find("Config");
    if (config_it == container_obj.end() || !config_it->second.is<picojson::value::object>()) {
        throw std::runtime_error("Config not found or not an object");
    }

    const auto &config_obj = config_it->second.get<picojson::value::object>();
    const auto labels_it = config_obj.find("Labels");
    if (labels_it == config_obj.end() || !labels_it->second.is<picojson::value::object>()) {
        throw std::runtime_error("Labels not found or not an object");
    }

    const auto &labels_obj = labels_it->second.get<picojson::value::object>();
    const auto path_it = labels_obj.find("com.docker.compose.project.working_dir");
    if (path_it == labels_obj.end() || !path_it->second.is<std::string>()) {
        throw std::runtime_error("Working directory label not found. Is the docker container based on a local image?");
    }

    return std::filesystem::path(path_it->second.get<std::string>());
}

std::string GickerCli::getContainerImageId(const picojson::value& json) {
    // Find "ImageId"

    const auto &arr = json.get<picojson::value::array>();
    if (arr.empty()) {
        throw std::runtime_error("Empty array from docker inspect");
    }

    const auto &image = arr[0];
    if (!image.is<picojson::value::object>()) {
        throw std::runtime_error("Expected container object");
    }

    const auto &image_obj = image.get<picojson::value::object>();
    const auto image_id_it = image_obj.find("Image");
    if (image_id_it == image_obj.end() || !image_id_it->second.is<std::string>()) {
        throw std::runtime_error("Image not found or not a string");
    }
    const auto image_id_obj = image_id_it->second.get<std::string>();

    return image_id_obj;
}

std::time_t GickerCli::getImageCreationTime(const picojson::value &json) {
    // Find "Created"

    const auto &arr = json.get<picojson::value::array>();
    if (arr.empty()) {
        throw std::runtime_error("Empty array from docker inspect");
    }

    const auto &container = arr[0];
    if (!container.is<picojson::value::object>()) {
        throw std::runtime_error("Expected container object");
    }

    const auto &container_obj = container.get<picojson::value::object>();
    const auto created_it = container_obj.find("Created");
    if (created_it == container_obj.end() || !created_it->second.is<std::string>()) {
        throw std::runtime_error("Created not found or not a string");
    }
    const auto created_obj = created_it->second.get<std::string>();

    // docker image inspect format:   "Created": "2025-11-04T18:39:50.016834308+01:00"
    // Cut the sub-seconds, but leave in the timezone
    std::string datetime_str = created_obj;

    if (const size_t dot_pos = datetime_str.find('.'); dot_pos != std::string::npos) {
        std::string to_cut = datetime_str.substr(dot_pos, datetime_str.length());
        int split_pos = 0;
        for (const char c : to_cut) {
            if (!std::isdigit(c) && c != '.') {
                break;
            }
            split_pos++;
        }
        to_cut.erase(split_pos, to_cut.length());

        datetime_str = datetime_str.erase(dot_pos, to_cut.length());
    }
    // now in format "2025-11-04T18:39:50+01:00"

    tm tm = {};
    if (strptime(datetime_str.c_str(), "%Y-%m-%dT%H:%M:%S%z", &tm) == nullptr) {
        throw std::runtime_error("Failed to parse creation time");
    }

    const std::time_t local_result = mktime(&tm);
    return local_result;
}

std::string GickerCli::getContainerBranch(const std::time_t &image_creation_time,
                                          const std::filesystem::path &container_source_path) {
    const std::string command = "git -C " + container_source_path.string() +
        " reflog show --before=" + std::to_string(image_creation_time) +
        " | grep \": checkout\"";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    // --before=<date> ensures that the uppermost line shows entry for the desired timestamp
    // expect output like: "4362b76 HEAD@{2}: checkout: moving from main to test-branch"
    // so just parse the very last word of the first line found
    char buffer[1024] = {0};
    if (!fgets(buffer, sizeof(buffer), pipe)) {
        pclose(pipe);
        throw std::runtime_error("No checkout entries found in reflog");
    }

    pclose(pipe);
    const std::string reflog_topline(buffer);

    return reflog_topline.substr(reflog_topline.find_last_of(" "), reflog_topline.length());
}

int main(const int argc, char *argv[]) {
    try {
        if (argc != 2) { throw std::runtime_error("Invalid number of arguments."); }

        const std::string container_id = argv[1];

        if (!(GickerCli::isDockerInstalled() && GickerCli::isGitInstalled())) {
            const std::string missing = GickerCli::isDockerInstalled() ? "git" : "Docker";
            throw std::runtime_error(missing + " not installed");
        }

        const picojson::value container_metadata = GickerCli::getDockerMetaData(
            container_id, GickerCli::container);
        const picojson::value image_metadata = GickerCli::getDockerMetaData(
            GickerCli::getContainerImageId(container_metadata), GickerCli::image);

        std::cout << GickerCli::getContainerBranch(
            GickerCli::getImageCreationTime(image_metadata),
            GickerCli::getContainerSourcePath(container_metadata)
            );
    } catch (std::runtime_error err) {
        std::cout << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
