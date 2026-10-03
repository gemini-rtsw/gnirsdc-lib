# gnirsdc-lib: pybind11 extension (libgnirsioc) that drives the GNIRS DC ARC
# controller, plus the prebuilt ARC API libraries it links against.
#
# Built from source by gemini-rtsw-ci (profile: lightweight, EL9).
#
# For EL9's system Python 3.9, which the GNIRS DC image runs (see
# gnirsdc-data-manager); a pybind11 module only loads into the interpreter it
# was built for.

%global specver 0.2.0
# $GIT_HASH first: build_rpm.sh computes it on the host and passes it in; the
# rpmbuild stage runs from a tarball with no .git.
%define git_hash %(if [ -n "$GIT_HASH" ]; then echo "$GIT_HASH"; else git rev-parse --short HEAD 2>/dev/null || echo nogit; fi)

%global py39_sitearch /usr/lib64/python3.9/site-packages
# The ARC libraries (and the cfitsio build they need) live in a private
# directory, found through libgnirsioc's RPATH, so they cannot collide with a
# distro cfitsio in /usr/lib64.
%global arclibdir %{_libdir}/%{name}

# Keep the private libraries out of the global Provides/Requires: nothing else
# should resolve libcfitsio.so.9 against this package.
%global __provides_exclude_from ^%{arclibdir}/.*$
%global __requires_exclude ^lib(CArc[A-Za-z]+|cfitsio)\\.so.*$

# The ARC libraries are shipped prebuilt; stripping or debuginfo extraction
# would rewrite vendor binaries.
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

Name:           gnirsdc-lib
Version:        %{specver}
Release:        1.git%{git_hash}%{?dist}
Summary:        GNIRS DC ARC controller library (libgnirsioc Python extension)
License:        Proprietary
Source0:        %{name}-%{version}.tar.gz
ExclusiveArch:  x86_64

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  git
BuildRequires:  python3-devel
BuildRequires:  pybind11-devel
BuildRequires:  json-c-devel
BuildRequires:  libuuid-devel

Requires:       python3

%description
libgnirsioc, the pybind11 extension the GNIRS detector controller IOC uses to
talk to the SDSU/ARC controller over PCIe, with the ARC API libraries it links
against. Installs the module into the Python 3.9 site-packages.

%prep
%setup -q
# build_rpm.sh builds the source tarball from regular files only, so the
# cfitsio soname symlinks in lib/ do not survive it. Recreate them: without
# libcfitsio.so the link silently falls back to the static libcfitsio.a, and
# without libcfitsio.so.9 the installed module cannot be imported.
ln -sf libcfitsio.so.9.3.48 lib/libcfitsio.so.9
ln -sf libcfitsio.so.9 lib/libcfitsio.so

%build
# GIT_DESCRIBE: there is no .git here, so hand the Makefile the commit for the
# DCVER header keyword. LDFLAGS: point the RPATH at the installed private
# library directory instead of $ORIGIN, since the module and the ARC libraries
# are installed to different places.
make distclean
make libgnirsioc.so \
    GIT_DESCRIBE=%{version}-%{git_hash} \
    LDFLAGS="-L./lib -Wl,-rpath,%{arclibdir} -shared"

%install
install -Dpm 0755 libgnirsioc.so %{buildroot}%{py39_sitearch}/libgnirsioc.so
install -dm 0755 %{buildroot}%{arclibdir}
# Shared objects only (cp -P keeps the cfitsio soname symlinks); the static
# libcfitsio.a is a build input, not a runtime file.
cp -P lib/*.so lib/*.so.* %{buildroot}%{arclibdir}/
install -Dpm 0644 libgnirsioc.h %{buildroot}%{_includedir}/%{name}/libgnirsioc.h

%check
# Fail the build, not the IOC: the RPATH must point at the installed library
# directory, and the module must import with the libraries as installed.
readelf -d libgnirsioc.so | grep -qE "R(UN)?PATH.*%{arclibdir}" || \
    { echo "ERROR: libgnirsioc.so has no RPATH to %{arclibdir}" >&2; exit 1; }
LD_LIBRARY_PATH=%{buildroot}%{arclibdir} PYTHONPATH=%{buildroot}%{py39_sitearch} \
    python3.9 -c "import libgnirsioc; libgnirsioc.controllerInterfaceDebug"

%files
%{py39_sitearch}/libgnirsioc.so
%{arclibdir}
%{_includedir}/%{name}

%changelog
* Fri Oct 02 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 0.2.0-1
- Build with gemini-rtsw-ci on GitHub, for EL9. Built from source; installs the
  module into Python 3.9 site-packages and the ARC libraries into a private
  directory.

* Mon Jun 06 2022 Hawi Stecher <hstecher@gemini.edu> 0.0.1-2
- new package built with tito
