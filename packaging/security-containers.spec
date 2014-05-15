%define script_dir %{_sbindir}

Name:          security-containers
Version:       0.1.0
Release:       0
Source0:       %{name}-%{version}.tar.gz
License:       Apache-2.0
Group:         Security/Other
Summary:       Daemon for managing containers
BuildRequires: cmake
BuildRequires: libvirt-devel
BuildRequires: libjson-devel
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libsystemd-journal)

%description
This package provides a daemon used to manage containers - start, stop and switch
between them. A process from inside a container can request a switch of context
(display, input devices) to the other container.

%files
%manifest packaging/security-containers.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/security-containers-server
%dir /etc/security-containers
%dir /etc/security-containers/containers
%dir /etc/security-containers/libvirt-config
%config /etc/security-containers/daemon.conf
%config /etc/security-containers/containers/*.conf
%config %attr(400,root,root) /etc/security-containers/libvirt-config/*.xml
/etc/security-containers/image-skel

%prep
%setup -q

%build
%{!?build_type:%define build_type "RELEASE"}

%if %{build_type} == "DEBUG" || %{build_type} == "PROFILING"
    CFLAGS="$CFLAGS -Wp,-U_FORTIFY_SOURCE"
    CXXFLAGS="$CXXFLAGS -Wp,-U_FORTIFY_SOURCE"
%endif

%cmake . -DVERSION=%{version} \
         -DCMAKE_BUILD_TYPE=%{build_type} \
         -DSCRIPT_INSTALL_DIR=%{script_dir}
make -k %{?jobs:-j%jobs}

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
%manifest packaging/libsecurity-containers-client.manifest
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
%manifest packaging/security-containers.manifest
%defattr(644,root,root,755)
%{_includedir}/security-containers
%{_libdir}/pkgconfig/*


## Container Daemon Package ####################################################
%package container-daemon
Summary:          Security Containers Containers Daemon
Group:            Security/Other
Requires:         security-containers = %{version}-%{release}
BuildRequires:    pkgconfig(glib-2.0)
BuildRequires:    pkgconfig(libsystemd-journal)

%description container-daemon
Daemon running inside every container.

%files container-daemon
%manifest packaging/security-containers-container-daemon.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/security-containers-container-daemon
/etc/dbus-1/system.d/com.samsung.container.daemon.conf


## Test Package ################################################################
%package unit-tests
Summary:          Security Containers Unit Tests
Group:            Development/Libraries
Requires:         security-containers = %{version}-%{release}
Requires:         security-containers-client = %{version}-%{release}
Requires:         python
Requires:         boost-test
BuildRequires:    boost-devel

%description unit-tests
Unit tests for both: server and client.

%files unit-tests
%manifest packaging/security-containers-server-unit-tests.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/security-containers-server-unit-tests
%attr(755,root,root) %{script_dir}/sc_all_tests.py
%attr(755,root,root) %{script_dir}/sc_launch_test.py
%{script_dir}/sc_test_parser.py
%{_datadir}/security-containers
