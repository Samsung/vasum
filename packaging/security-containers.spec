%define script_dir %{_libdir}/python/site-packages/sc_test_scripts/
%define script_src_dir src/scripts

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
BuildRequires: libjson
BuildRequires: libjson-devel

%description
This package provides a daemon used to manage containers - start, stop and switch
between them. A process from inside a container can request a switch of context
(display, input devices) to the other container.

%files
%attr(755,root,root) %{_bindir}/security-containers-server
%config %attr(644,root,root) /etc/security-containers/config/daemon.json
%config %attr(400,root,root) /etc/security-containers/config/libvirt-config/*.xml

%prep
%setup -q

%build
%{!?build_type:%define build_type "RELEASE"}

%if %{build_type} == "DEBUG"
    # workaround for a bug in build.conf
    %global optflags %(echo %{optflags} | sed 's/-Wp,-D_FORTIFY_SOURCE=2//')
    export CFLAGS="%{optflags}"
    export CXXFLAGS="%{optflags}"
%endif

%cmake . -DVERSION=%{version} \
         -DCMAKE_BUILD_TYPE=%{build_type}
make -k %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}/etc/security-containers/config/libvirt-config/

install -d %{buildroot}/%{_bindir}
install -d %{buildroot}/%{script_dir}
install -m 755 %{script_src_dir}/sc_tests_all.py %{buildroot}/%{script_dir}
install -m 755 %{script_src_dir}/sc_test_launch.py %{buildroot}/%{script_dir}
install -m 755 %{script_src_dir}/sc_test_parser.py %{buildroot}/%{script_dir}

ln -sf %{script_dir}/sc_tests_all.py %{buildroot}/%{_bindir}/sc_tests_all
ln -sf %{script_dir}/sc_test_launch.py %{buildroot}/%{_bindir}/sc_test_launch

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
%defattr(644,root,root,644)
%attr(755,root,root) %{_bindir}/security-containers-server-unit-tests
%attr(755,root,root) %{script_dir}/sc_tests_all.py
%attr(755,root,root) %{script_dir}/sc_test_launch.py
%{script_dir}/sc_test_parser.py
%{_bindir}/sc_tests_all
%{_bindir}/sc_test_launch
%config %attr(644,root,root) /etc/security-containers/config/tests/ut-scs-container-manager/test-daemon.conf
%config %attr(644,root,root) /etc/security-containers/config/tests/ut-scs-container-manager/libvirt-config/*.xml
