%define script_dir %{_sbindir}
# Security Containers Server's user info - it should already exist in the system
%define scs_user          security-containers
# The group that has read and write access to /dev/input/event* devices.
# It may vary between platforms.
%define input_event_group input
# The group has access to /dev/loop* devices.
%define disk_group disk
# The group that has write access to /dev/tty* devices.
%define tty_group tty

Name:           security-containers
Version:        0.1.1
Release:        0
Source0:        %{name}-%{version}.tar.gz
License:        Apache-2.0
Group:          Security/Other
Summary:        Daemon for managing containers
BuildRequires:  cmake
BuildRequires:  boost-devel
BuildRequires:  libjson-devel >= 0.10
BuildRequires:  libcap-ng-devel
BuildRequires:  lxc-devel
BuildRequires:  pkgconfig(libConfig)
BuildRequires:  pkgconfig(libLogger)
BuildRequires:  pkgconfig(libSimpleDbus)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libsystemd-journal)
BuildRequires:  pkgconfig(sqlite3)
Requires(post): libcap-tools

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
%dir /etc/security-containers/lxc-templates
%config /etc/security-containers/daemon.conf
%config /etc/security-containers/containers/*.conf
%attr(755,root,root) /etc/security-containers/lxc-templates/*.sh
%{_unitdir}/security-containers.service
%{_unitdir}/multi-user.target.wants/security-containers.service
/etc/dbus-1/system.d/org.tizen.containers.host.conf

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
         -DSCRIPT_INSTALL_DIR=%{script_dir} \
         -DSYSTEMD_UNIT_DIR=%{_unitdir} \
         -DPYTHON_SITELIB=%{python_sitelib} \
         -DSECURITY_CONTAINERS_USER=%{scs_user} \
         -DINPUT_EVENT_GROUP=%{input_event_group} \
         -DDISK_GROUP=%{disk_group} \
         -DTTY_GROUP=%{tty_group}
make -k %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}/%{_unitdir}/multi-user.target.wants
ln -s ../security-containers.service %{buildroot}/%{_unitdir}/multi-user.target.wants/security-containers.service

%clean
rm -rf %{buildroot}

%post
# Refresh systemd services list after installation
if [ $1 == 1 ]; then
    systemctl daemon-reload || :
fi
# set needed caps on the binary to allow restart without loosing them
setcap CAP_SYS_ADMIN,CAP_MAC_OVERRIDE,CAP_SYS_TTY_CONFIG+ei %{_bindir}/security-containers-server

%preun
# Stop the service before uninstall
if [ $1 == 0 ]; then
     systemctl stop security-containers.service || :
fi

%postun
# Refresh systemd services list after uninstall/upgrade
systemctl daemon-reload || :
if [ $1 -ge 1 ]; then
    # TODO: at this point an appropriate notification should show up
    eval `systemctl show security-containers --property=MainPID`
    if [ -n "$MainPID" -a "$MainPID" != "0" ]; then
        kill -USR1 $MainPID
    fi
    echo "Security Containers updated. Reboot is required for the changes to take effect..."
else
    echo "Security Containers removed. Reboot is required for the changes to take effect..."
fi

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
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/libsecurity-containers.so.0.0.1
%{_libdir}/libsecurity-containers.so.0

%post client -p /sbin/ldconfig

%postun client -p /sbin/ldconfig


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
%{_libdir}/libsecurity-containers.so
%{_includedir}/security-containers
%{_libdir}/pkgconfig/*.pc


## Container Support Package ###################################################
# TODO move to a separate repository
%package container-support
Summary:          Security Containers Support
Group:            Security/Other
Conflicts:        security-containers

%description container-support
Containers support installed inside every container.

%files container-support
%manifest packaging/security-containers-container-support.manifest
%defattr(644,root,root,755)
/etc/dbus-1/system.d/org.tizen.containers.domain.conf


## Container Daemon Package ####################################################
# TODO move to a separate repository
%package container-daemon
Summary:          Security Containers Containers Daemon
Group:            Security/Other
Requires:         security-containers-container-support = %{version}-%{release}

%description container-daemon
Daemon running inside every container.

%files container-daemon
%manifest packaging/security-containers-container-daemon.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/security-containers-container-daemon
/etc/dbus-1/system.d/org.tizen.containers.domain.daemon.conf


## Command Line Interface ######################################################
%package cli
Summary:          Security Containers Command Line Interface
Group:            Security/Other
Requires:         security-containers-client = %{version}-%{release}

%description cli
Command Line Interface for security-containers.

%files cli
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/security-containers-cli


## Test Package ################################################################
%package tests
Summary:          Security Containers Tests
Group:            Development/Libraries
Requires:         security-containers = %{version}-%{release}
Requires:         security-containers-client = %{version}-%{release}
Requires:         python
Requires:         python-xml
Requires:         boost-test

%description tests
Unit tests for both: server and client and integration tests.

%files tests
%manifest packaging/security-containers-server-tests.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/security-containers-server-unit-tests
%attr(755,root,root) %{script_dir}/sc_all_tests.py
%attr(755,root,root) %{script_dir}/sc_int_tests.py
%attr(755,root,root) %{script_dir}/sc_launch_test.py
%{script_dir}/sc_test_parser.py
%{_datadir}/security-containers/tests
%attr(755,root,root) %{_datadir}/security-containers/lxc-templates
%{python_sitelib}/sc_integration_tests
/etc/dbus-1/system.d/org.tizen.containers.tests.conf
