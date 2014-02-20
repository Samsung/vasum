Name:          security-containers
Version:       0.1.0
Release:       0
Source0:       %{name}-%{version}.tar.gz
License:       Apache-2.0
Group:         Security/Other
Summary:       Daemon for managing containers
BuildRequires: cmake
BuildRequires: libvirt
BuildRequires: libvirt-devel

%description
This package provides a daemon used to manage containers - start, stop and switch
between them. A proccess from inside a container can request a switch of context
(display, inputdevices) to the other container.

%files
%attr(755,root,root) %{_bindir}/security-containers-server

%prep
%setup -q

%build
%cmake . -DVERSION=%{version} \
         -DCMAKE_BUILD_TYPE=%{?build_type:%build_type}%{!?build_type:RELEASE} \
         -DCMAKE_VERBOSE_MAKEFILE=ON
make %{?jobs:-j%jobs}

%install
%make_install

%clean
rm -rf %{buildroot}


## Client Package ##############################################################
%package client
Summary:          Security Containers Client
Group:            Development/Libraries
Requires:         security-containers = %{version}-%{release}
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description client
Library interface to the security-containers daemon

%files client
%attr(644,root,root) %{_libdir}/libsecurity-containers-client.so


## Devel Package ###############################################################
%package devel
Summary:          Security Containers Client Devel
Group:            Development/Libraries
Requires:         security-containers = %{version}-%{release}
Requires:         security-containers-client = %{version}-%{release}

%description devel
Development package including the header files for the client library

%files devel
%defattr(644,root,root,644)
%attr(644,root,root) %{_includedir}/security-containers/security-containers-client.h
%attr(644,root,root) %{_libdir}/pkgconfig/security-containers.pc
