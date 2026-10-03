#!/bin/bash
# Run by gemini-rtsw-ci/build_rpm.sh just before `dnf builddep`.
#
# The lightweight profile resolves BuildRequires from the base Rocky repos only.
# This package also needs:
#   - the python39 module stream (python39-devel) and the python39-devel stream
#     in powertools (pybind11-devel, as python39-pybind11-devel): modular
#     packages are filtered out until their stream is enabled
#   - EPEL, which the powertools content leans on
set -euo pipefail
dnf -y install epel-release dnf-plugins-core
dnf config-manager --set-enabled powertools
dnf -y module enable python39 python39-devel
