# gicker

## What is gicker?

A simple utility-tool collection to make Docker and git work together nicely.
Currently only tested under Linux.

## Functionality

At the moment the only function is to feed _gicker_ a Docker container ID of a container that has been build from a local Docker image. It outputs the git-branch that the image has been build from based on Docker's timestamps and `git-reflog`.

```bash
gicker 3e341456be7b
feature-branch-123
```

## Install

Requires:
- CMake
- vcpkg

Install via 
```bash
cmake -B build .
```