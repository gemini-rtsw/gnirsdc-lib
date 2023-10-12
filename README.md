# gnirsdc-lib Docker Environment

## Overview

This repository contains a Docker environment for the `gnirsdc-lib` library. The image is based on Rocky Linux 9 and includes development packages for JSON, UUID, Pybind11, Python 3, and the GNU Compiler Collection.

## Building the Docker Image

Navigate to the root directory of the `gnirsdc-lib` repository and execute the following command to build the Docker image:

```bash
docker build -t gnirsdc-lib-env .
```

For a Rocky Linux 8 image:

```bash
docker build -t gnirsdc-lib-env -f ./Dockerfile_rocky8 .
```
## Compiling the Library

After building the image, you can compile the library using:

```bash
docker run --rm -v $(pwd):/work/$(basename $(pwd)) -it gnirsdc-lib-env bash -l -c "cd /work/$(basename $(pwd)) && make clean all"
```

This will mount the current working directory to a corresponding directory in the container, navigate to that directory, and run `make clean all`.

## Python Environment Setup

To use the compiled library in a Python environment, you can install it using pip:

```bash
pip install .
```

Usage:

```bash
python
>>> import libgnirsioc
>>> controller = libgnirsioc.controllerInterface()
```

## Dependencies

- `json-c-devel`: Development files for JSON-C, a JSON implementation in C.
- `libuuid-devel`: Development files for Universally Unique Identifier library.
- `pybind11-devel`: Development files for pybind11, a library for creating Python bindings to C++ code.
- `python3-devel`: Development files and libraries for Python 3.
- `g++`: GNU C++ compiler.
- `make`: GNU make utility to maintain groups of programs.

These dependencies are automatically installed when building the Docker image.