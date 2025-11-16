//
// Created by simon on 06.11.25.
//

#ifndef GICKER_GICKER_CLI_H
#define GICKER_GICKER_CLI_H
#include <picojson/picojson.h>
#include <string>
#include <filesystem>

namespace GickerCli {
    enum DockerElem { image, container };

    inline std::string toString(const DockerElem docker_elem) {
        switch (docker_elem)
        {
            case image:   return "image";
            case container:   return "container";
            default:      return "[Unknown DockerElem]";
        }
    }


    /**
     * Check if Docker is installed on the system.
     * @return true if Docker is installed
     */
    bool isDockerInstalled();

    /**
     * Check if git is installed on the system.
     * @return true if git is installed
     */
    bool isGitInstalled();

    /**
     * Checks if a command is available on the system. Assumes POSIX-conformity
     * @param command
     * @return true if command is available
     */
    bool isCommandInstalled(const std::string& command);

    /**
     * Get a JSON of docker metadata
     * @param id
     * @param docker_elem image or container
     * @return
     */
    picojson::value getDockerMetaData(const std::string& id, DockerElem docker_elem);

    /**
     * Get the source directory of a Docker container from its metadata
     * @param json output of 'docker container inspect'
     * @return Source directory path
     */
    std::filesystem::path getContainerSourcePath(const picojson::value& json);

    /**
     * Get the source ImageId of a Docker container from its metadata
     * @param json output of 'docker image inspect'
     * @return
     */
    std::string getContainerImageId(const picojson::value& json);

    /**
     * Get the time of creation of a Docker image from its metadata
     * @param json output of 'docker image inspect'
     * @return time of creation of a Docker image
     */
    std::time_t getImageCreationTime(const picojson::value& json);

    /**
     * Get the git-branch that a local Docker container has been created from
     * @param image_creation_time
     * @param container_source_path
     * @return git-branch name
     */
    std::string getContainerBranch(const std::time_t& image_creation_time,
                                   const std::filesystem::path& container_source_path);

};

#endif //GICKER_GICKER_CLI_H
