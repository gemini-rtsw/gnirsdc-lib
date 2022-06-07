%define _prefix /gem_base/epics/support
%define name gnirsdc-lib 
%define repository gemdev
%define debug_package %{nil}
%define arch %(uname -m)
%define checkout %(git log --pretty=format:'%h' -n 1) 

#These global defines are added to prevent stripping
# symbols on vxWorks cross-compiled code
# Getting 'strip' to work is probably only needed for
# building a related debug sub-package
#
# But this prevents all the strip warnings
# mrippa 20120202
%global _enable_debug_package 0
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

Summary: %{name} Package, library for gnirsDC ARC controller 
Name: %{name}
Version: 0.0.1
Release: 1%{?dist}
License: EPICS Open License
Group: Applications/Engineering
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: %{arch}
Prefix: %{_prefix}
## You may specify dependencies here
BuildRequires: json-c-devel libuuid-devel 
Requires: json-c-devel libuuid-devel gnirsdc-dsp
## Switch dependency checking off
## AutoReqProv: no

%description
This is the library %{name}.

## If you want to have a devel-package to be generated uncomment the following:
%package devel
Summary: %{name}-devel Package
Group: Development/Gemini
Requires: %{name}
%description devel
This is the library %{name}.

%prep
%setup -q 

%build
make distclean uninstall
make

%install
export DONT_STRIP=1
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{_prefix}/%{name}
mkdir -p $RPM_BUILD_ROOT/%{_prefix}/%{name}
mkdir -p $RPM_BUILD_ROOT/%{_prefix}/%{name}
cp -r release/include $RPM_BUILD_ROOT/%{_prefix}/%{name}/include
cp -r release/lib $RPM_BUILD_ROOT/%{_prefix}/%{name}/lib
cp -r include $RPM_BUILD_ROOT/%{_prefix}/%{name}/include
cp -r lib $RPM_BUILD_ROOT/%{_prefix}/%{name}/lib


%postun
if [ "$1" = "0" ]; then
	rm -rf %{_prefix}/%{name}
fi


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
   /%{_prefix}/%{name}/include
   /%{_prefix}/%{name}/lib

%files devel
%defattr(-,root,root)
   /%{_prefix}/%{name}/include
   /%{_prefix}/%{name}/lib

%changelog
* Wed Apr 27 2022 Hawi Stecher <hstecher@gemini.edu> 0.0.1-1
- new package built with tito


