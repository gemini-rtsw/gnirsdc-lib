#!/bin/bash
set -e

echo "Building gnirsdc-lib Docker environment for Rocky Linux 9..."

# Build the Docker image
docker build -t gnirsdc-lib-env .

# Compile the library
docker run --rm -v $(pwd):/work/$(basename $(pwd)) -it gnirsdc-lib-env bash -l -c "cd /work/$(basename $(pwd)) && make clean all"

echo "Build complete! You can now install the library with: pip install ." 