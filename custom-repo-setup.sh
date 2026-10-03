#!/bin/bash
# Run by gemini-rtsw-ci/build_rpm.sh just before `dnf builddep`.
#
# The lightweight profile resolves BuildRequires from the base Rocky repos only;
# pybind11-devel is in EPEL, which needs CRB enabled on EL9.
set -euo pipefail
dnf -y install epel-release dnf-plugins-core
dnf config-manager --set-enabled crb
