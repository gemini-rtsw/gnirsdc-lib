# Usage
# Build:
# cd <gnirsdc-lib repo base>
# docker build -t gnirsdc-lib-env .
# docker run --rm -v $(pwd):/work/$(basename $(pwd)) -it gnirsdc-lib-env bash -l -c "cd /work/$(basename $(pwd)) && make clean all"
#
# Use in python environment: 
# pip install .
#
# Use Rocky Linux 9 as the base image
FROM rockylinux/rockylinux:9

# Metadata as described
LABEL maintainer="hawi.stecher@noirlab.edu"
LABEL date="2023-10-11"

# Update the package index and install required packages
RUN dnf install -y epel-release && \
    dnf -y update && \
    dnf -y config-manager --set-enabled crb && \
    dnf -y install json-c-devel libuuid-devel pybind11-devel python3-devel g++ make




