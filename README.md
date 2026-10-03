# gnirsdc-lib

`libgnirsioc`, the pybind11 extension the GNIRS detector controller IOC uses to
drive the SDSU/ARC controller over PCIe, together with the prebuilt ARC API
libraries it links against (`lib/`, headers in `include/`, source in `ARC_API/`).

Built and published by [gemini-rtsw-ci](https://github.com/gemini-rtsw/gemini-rtsw-ci):
every push to `main` builds the `gnirsdc-lib` RPM (EL8, Python 3.9) and
publishes it to the shared rpm-repo. The GNIRS DC image gets it from there
through `gnirsdc-data-manager`'s spec.

## What the RPM installs

| path | what |
|---|---|
| `/usr/lib64/python3.9/site-packages/libgnirsioc.so` | the Python module |
| `/usr/lib64/gnirsdc-lib/` | ARC libraries and cfitsio, found through the module's RPATH |
| `/usr/include/gnirsdc-lib/libgnirsioc.h` | header |

```python
>>> import libgnirsioc
>>> controller = libgnirsioc.controllerInterface()
```

## Developing

```bash
git clone --recurse-submodules git@github.com:gemini-rtsw/gnirsdc-lib.git
cd gnirsdc-lib
./gemini-rtsw-ci/dev_environment.sh --el 8   # the environment CI builds in
make                                          # inside the container
```

`make` writes `libgnirsioc.so` and a `release/` tree in the checkout; neither is
committed.

To build the RPM exactly as CI does:

```bash
./gemini-rtsw-ci/build_rpm.sh --el 8 --profile lightweight    # RPM lands in rpms/
```

Open a pull request to have CI build it; merging to `main` publishes it. See the
gemini-rtsw-ci [WORKFLOW.md](https://github.com/gemini-rtsw/gemini-rtsw-ci/blob/main/WORKFLOW.md).

## Dependencies

Build: `gcc-c++`, `python39-devel`, `pybind11-devel`, `json-c-devel`,
`libuuid-devel`. On EL8 those need EPEL, powertools and the `python39` /
`python39-devel` module streams, which `custom-repo-setup.sh` enables.
