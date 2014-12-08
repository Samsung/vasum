%define script_dir %{_sbindir}
# Vasum Server's user info - it should already exist in the system
%define vsm_user          security-containers
# The group that has read and write access to /dev/input/event* devices.
# It may vary between platforms.
%define input_event_group input
# The group has access to /dev/loop* devices.
%define disk_group disk
# The group that has write access to /dev/tty* devices.
%define tty_group tty

Name:           vasum
Version:        0.1.1
Release:        0
Source0:        %{name}-%{version}.tar.gz
License:        Apache-2.0
Group:          Security/Other
Summary:        Daemon for managing zones
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
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(sqlite3)
Requires(post): libcap-tools
Requires:       bridge-utils

%description
This package provides a daemon used to manage zones - start, stop and switch
between them. A process from inside a zone can request a switch of context
(display, input devices) to the other zone.

%files
%manifest packaging/vasum.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-server
%dir /etc/vasum
%dir /etc/vasum/zones
%dir /etc/vasum/lxc-templates
%dir /etc/vasum/templates
%config /etc/vasum/daemon.conf
%config /etc/vasum/zones/*.conf
%attr(755,root,root) /etc/vasum/lxc-templates/*.sh
%config /etc/vasum/templates/*.conf
%{_unitdir}/vasum.service
%{_unitdir}/multi-user.target.wants/vasum.service
/etc/dbus-1/system.d/org.tizen.vasum.host.conf

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
         -DVASUM_USER=%{vsm_user} \
         -DINPUT_EVENT_GROUP=%{input_event_group} \
         -DDISK_GROUP=%{disk_group} \
         -DTTY_GROUP=%{tty_group}
make -k %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}/%{_unitdir}/multi-user.target.wants
ln -s ../vasum.service %{buildroot}/%{_unitdir}/multi-user.target.wants/vasum.service

%clean
rm -rf %{buildroot}

%post
# Refresh systemd services list after installation
if [ $1 == 1 ]; then
    systemctl daemon-reload || :
fi
# set needed caps on the binary to allow restart without loosing them
setcap CAP_SYS_ADMIN,CAP_MAC_OVERRIDE,CAP_SYS_TTY_CONFIG+ei %{_bindir}/vasum-server

%preun
# Stop the service before uninstall
if [ $1 == 0 ]; then
     systemctl stop vasum.service || :
fi

%postun
# Refresh systemd services list after uninstall/upgrade
systemctl daemon-reload || :
if [ $1 -ge 1 ]; then
    # TODO: at this point an appropriate notification should show up
    eval `systemctl show vasum --property=MainPID`
    if [ -n "$MainPID" -a "$MainPID" != "0" ]; then
        kill -USR1 $MainPID
    fi
    echo "Vasum updated. Reboot is required for the changes to take effect..."
else
    echo "Vasum removed. Reboot is required for the changes to take effect..."
fi

## Client Package ##############################################################
%package client
Summary:          Vasum Client
Group:            Development/Libraries
Requires:         vasum = %{version}-%{release}
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description client
Library interface to the vasum daemon

%files client
%manifest packaging/libvasum-client.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/libvasum.so.0.0.1
%{_libdir}/libvasum.so.0

%post client -p /sbin/ldconfig

%postun client -p /sbin/ldconfig


## Devel Package ###############################################################
%package devel
Summary:          Vasum Client Devel
Group:            Development/Libraries
Requires:         vasum = %{version}-%{release}
Requires:         vasum-client = %{version}-%{release}

%description devel
Development package including the header files for the client library

%files devel
%manifest packaging/vasum.manifest
%defattr(644,root,root,755)
%{_libdir}/libvasum.so
%{_includedir}/vasum
%{_libdir}/pkgconfig/*.pc


## Zone Support Package ###################################################
# TODO move to a separate repository
%package zone-support
Summary:          Vasum Support
Group:            Security/Other
Conflicts:        vasum

%description zone-support
Zones support installed inside every zone.

%files zone-support
%manifest packaging/vasum-zone-support.manifest
%defattr(644,root,root,755)
/etc/dbus-1/system.d/org.tizen.vasum.zone.conf


## Zone Daemon Package ####################################################
# TODO move to a separate repository
%package zone-daemon
Summary:          Vasum Zones Daemon
Group:            Security/Other
Requires:         vasum-zone-support = %{version}-%{release}

%description zone-daemon
Daemon running inside every zone.

%files zone-daemon
%manifest packaging/vasum-zone-daemon.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-zone-daemon
/etc/dbus-1/system.d/org.tizen.vasum.zone.daemon.conf


## Command Line Interface ######################################################
%package cli
Summary:          Vasum Command Line Interface
Group:            Security/Other
Requires:         vasum-client = %{version}-%{release}

%description cli
Command Line Interface for vasum.

%files cli
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-cli


## Test Package ################################################################
%package tests
Summary:          Vasum Tests
Group:            Development/Libraries
Requires:         vasum = %{version}-%{release}
Requires:         vasum-client = %{version}-%{release}
Requires:         python
Requires:         python-xml
Requires:         boost-test

%description tests
Unit tests for both: server and client and integration tests.

%files tests
%manifest packaging/vasum-server-tests.manifest
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-server-unit-tests
%attr(755,root,root) %{script_dir}/vsm_all_tests.py
%attr(755,root,root) %{script_dir}/vsm_int_tests.py
%attr(755,root,root) %{script_dir}/vsm_launch_test.py
%{script_dir}/vsm_test_parser.py
%{_datadir}/vasum/tests
%attr(755,root,root) %{_datadir}/vasum/lxc-templates
%{python_sitelib}/vsm_integration_tests
/etc/dbus-1/system.d/org.tizen.vasum.tests.conf
