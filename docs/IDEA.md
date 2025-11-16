# gicker

## Core idea

* `git reflog` can be used to see when a branch has been checked out
  * Need to read into this further
* Then, this information can be compared with the data from `docker inpect <IMAGE>` to see which branch the image is based on
  * Could also give the possibility to show last commit

## GUI assets and components

* List of
  * Docker containers
  * Docker images
  * ...
* Use color scheme of docker
* Logo could be some union of the docker and git logo

## Tech stack

* C++
* docker-CLI
* git

Target OS: Primarily Linux, but Windows and MacOS should also easily be possibly
