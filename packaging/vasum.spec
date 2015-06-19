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
# Default platform is Tizen, setup Fedora with --define 'platform_type FEDORA'
%{!?platform_type:%define platform_type "TIZEN"}

Name:           vasum
Epoch:          1
Version:        0.1.1
Release:        0
Source0:        %{name}-%{version}.tar.gz
License:        Apache-2.0
Group:          Security/Other
Summary:        Daemon for managing zones
BuildRequires:  cmake
BuildRequires:  boost-devel
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  lxc-devel
Requires:       lxc
%if %{platform_type} == "TIZEN"
Requires:       iproute2
Requires(post): libcap-tools
%else
Requires:       lxc-templates
Requires:       iproute
Requires(post): libcap
%endif
Obsoletes:      vasum-daemon < 1:0

%description
This package provides a daemon used to manage zones - start, stop and switch
between them. A process from inside a zone can request a switch of context
(display, input devices) to the other zone.

%files
%if %{platform_type} == "TIZEN"
%manifest packaging/vasum.manifest
%endif
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-server
%attr(755,root,root) %{_sbindir}/vasum-check-config
%dir /etc/vasum
%dir /etc/vasum/templates
%config /etc/vasum/daemon.conf
%attr(755,root,root) /etc/vasum/templates/*.sh
%config /etc/vasum/templates/*.conf
%{_unitdir}/vasum.service
%{_unitdir}/vasum.socket
%{_unitdir}/multi-user.target.wants/vasum.service
%config /etc/dbus-1/system.d/org.tizen.vasum.host.conf
%dir %{_datadir}/zones

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
         -DDATA_DIR=%{_datadir} \
         -DPYTHON_SITELIB=%{python_sitelib} \
         -DVASUM_USER=%{vsm_user} \
         -DINPUT_EVENT_GROUP=%{input_event_group} \
         -DDISK_GROUP=%{disk_group} \
         -DTTY_GROUP=%{tty_group} \
         -DWITHOUT_DBUS=%{?without_dbus}
make -k %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}/%{_unitdir}/multi-user.target.wants
ln -s ../vasum.service %{buildroot}/%{_unitdir}/multi-user.target.wants/vasum.service
mkdir -p %{buildroot}/%{_datadir}/zones

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
Requires:         vasum = %{epoch}:%{version}-%{release}
Obsoletes:        vasum < 1:0
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description client
Library interface to the vasum daemon

%post -n vasum-client -p /sbin/ldconfig

%postun -n vasum-client -p /sbin/ldconfig

%files client
%if %{platform_type} == "TIZEN"
%manifest packaging/libvasum-client.manifest
%endif
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/libvasum-client.so.%{version}
%{_libdir}/libvasum-client.so.0
%attr(755,root,root) %{_libdir}/libvasum.so.%{version}
%{_libdir}/libvasum.so.0


## Devel Package ###############################################################
%package devel
Summary:          Vasum Client Devel
Group:            Development/Libraries
Requires:         vasum = %{epoch}:%{version}-%{release}
Requires:         vasum-client = %{epoch}:%{version}-%{release}

%description devel
Development package including the header files for the client library

%files devel
%if %{platform_type} == "TIZEN"
%manifest packaging/vasum.manifest
%endif
%defattr(644,root,root,755)
%{_libdir}/libvasum-client.so
%{_libdir}/libvasum.so
%{_includedir}/vasum
%{_libdir}/pkgconfig/vasum-client.pc
%{_libdir}/pkgconfig/vasum.pc


## Zone Support Package ###################################################
%package zone-support
Summary:          Vasum Support
Group:            Security/Other

%description zone-support
Zones support installed inside every zone.

%files zone-support
%if %{platform_type} == "TIZEN"
%manifest packaging/vasum-zone-support.manifest
%endif
%defattr(644,root,root,755)
%config /etc/dbus-1/system.d/org.tizen.vasum.zone.conf


## Zone Daemon Package ####################################################
%package zone-daemon
Summary:          Vasum Zones Daemon
Group:            Security/Other
Requires:         vasum-zone-support = %{epoch}:%{version}-%{release}

%description zone-daemon
Daemon running inside every zone.

%files zone-daemon
%if %{platform_type} == "TIZEN"
%manifest packaging/vasum-zone-daemon.manifest
%endif
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-zone-daemon
%config /etc/dbus-1/system.d/org.tizen.vasum.zone.daemon.conf


## Command Line Interface ######################################################
%package cli
Summary:          Vasum Command Line Interface
Group:            Security/Other
Requires:         vasum-client = %{epoch}:%{version}-%{release}

%description cli
Command Line Interface for vasum.

%files cli
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-cli

%package cli-completion
Summary:          Vasum Command Line Interface bash completion
Group:            Security/Other
Requires:         vasum-cli = %{epoch}:%{version}-%{release}
#Requires:         bash-completion

%description cli-completion
Command Line Interface bash completion for vasum.

%files cli-completion
%attr(755,root,root) %{_sysconfdir}/bash_completion.d/vasum-cli-completion.sh

## Test Package ################################################################
%package tests
Summary:          Vasum Tests
Group:            Development/Libraries
Requires:         vasum = %{epoch}:%{version}-%{release}
Requires:         vasum-client = %{epoch}:%{version}-%{release}
Requires:         python
%if %{platform_type} == "TIZEN"
Requires:         python-xml
%endif
Requires:         boost-test

%description tests
Unit tests for both: server and client and integration tests.

%post tests
systemctl daemon-reload
systemctl enable vasum-socket-test.socket
systemctl start vasum-socket-test.socket

%preun tests
systemctl stop vasum-socket-test.socket
systemctl disable vasum-socket-test.socket

%postun tests
systemctl daemon-reload

%files tests
%if %{platform_type} == "TIZEN"
%manifest packaging/vasum-server-tests.manifest
%endif
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-server-unit-tests
%attr(755,root,root) %{_bindir}/vasum-socket-test
%attr(755,root,root) %{script_dir}/vsm_all_tests.py
%attr(755,root,root) %{script_dir}/vsm_int_tests.py
%attr(755,root,root) %{script_dir}/vsm_launch_test.py
%{script_dir}/vsm_test_parser.py
%config /etc/vasum/tests/*.conf
%config /etc/vasum/tests/dbus/*.conf
%config /etc/vasum/tests/provision/*.conf
%config /etc/vasum/tests/templates/*.conf
%attr(755,root,root) /etc/vasum/tests/templates/*.sh
%config /etc/vasum/tests/utils/*.txt
%{python_sitelib}/vsm_integration_tests
%config /etc/dbus-1/system.d/org.tizen.vasum.tests.conf
%{_unitdir}/vasum-socket-test.socket
%{_unitdir}/vasum-socket-test.service

## libLogger Package ###########################################################
%package -n libLogger
Summary:            Logger library
Group:              Security/Other
BuildRequires:      pkgconfig(libsystemd-journal)
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libLogger
The package provides libLogger library.

%post -n libLogger -p /sbin/ldconfig

%postun -n libLogger -p /sbin/ldconfig

%files -n libLogger
%defattr(644,root,root,755)
%{_libdir}/libLogger.so.0
%attr(755,root,root) %{_libdir}/libLogger.so.%{version}

%package -n libLogger-devel
Summary:        Development logger library
Group:          Development/Libraries
Requires:       libLogger = %{epoch}:%{version}-%{release}

%description -n libLogger-devel
The package provides libLogger development tools and libs.

%files -n libLogger-devel
%defattr(644,root,root,755)
%{_libdir}/libLogger.so
%{_includedir}/vasum-tools/logger
%{_libdir}/pkgconfig/libLogger.pc

## libSimpleDbus Package #######################################################
%package -n libSimpleDbus
Summary:            Simple dbus library
Group:              Security/Other
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libSimpleDbus
The package provides libSimpleDbus library.

%post -n libSimpleDbus -p /sbin/ldconfig

%postun -n libSimpleDbus -p /sbin/ldconfig

%files -n libSimpleDbus
%defattr(644,root,root,755)
%{_libdir}/libSimpleDbus.so.0
%attr(755,root,root) %{_libdir}/libSimpleDbus.so.%{version}

%package -n libSimpleDbus-devel
Summary:        Development Simple dbus library
Group:          Development/Libraries
Requires:       libSimpleDbus = %{epoch}:%{version}-%{release}
Requires:       pkgconfig(libLogger)

%description -n libSimpleDbus-devel
The package provides libSimpleDbus development tools and libs.

%files -n libSimpleDbus-devel
%defattr(644,root,root,755)
%{_libdir}/libSimpleDbus.so
%{_includedir}/vasum-tools/dbus
%{_libdir}/pkgconfig/libSimpleDbus.pc

## libConfig Package ##########################################################
%package -n libConfig
Summary:            Config library
Group:              Security/Other
BuildRequires:      pkgconfig(sqlite3)
%if %{platform_type} == "TIZEN"
BuildRequires:      libjson-devel >= 0.10
Requires:           libjson >= 0.10
%else
BuildRequires:      json-c-devel
Requires:           json-c
%endif
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libConfig
The package provides libConfig library.

%post -n libConfig -p /sbin/ldconfig

%postun -n libConfig -p /sbin/ldconfig

%files -n libConfig
%defattr(644,root,root,755)
%{_libdir}/libConfig.so.0
%attr(755,root,root) %{_libdir}/libConfig.so.%{version}

%package -n libConfig-devel
Summary:        Development Config library
Group:          Development/Libraries
Requires:       libConfig = %{epoch}:%{version}-%{release}
Requires:       boost-devel
Requires:       pkgconfig(libLogger)
%if %{platform_type} == "TIZEN"
Requires:       libjson-devel >= 0.10
%else
Requires:       json-c-devel
%endif

%description -n libConfig-devel
The package provides libConfig development tools and libs.

%files -n libConfig-devel
%defattr(644,root,root,755)
%{_libdir}/libConfig.so
%{_includedir}/vasum-tools/config
%{_libdir}/pkgconfig/libConfig.pc

## libIpc Package #######################################################
%package -n libIpc
Summary:            IPC library
Group:              Security/Other
BuildRequires:      pkgconfig(libsystemd-daemon)
Requires:           libConfig
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libIpc
The package provides libIpc library.

%post -n libIpc -p /sbin/ldconfig

%postun -n libIpc -p /sbin/ldconfig

%files -n libIpc
%defattr(644,root,root,755)
%{_libdir}/libIpc.so.0
%attr(755,root,root) %{_libdir}/libIpc.so.%{version}

%package -n libIpc-devel
Summary:        Development IPC library
Group:          Development/Libraries
Requires:       libIpc = %{epoch}:%{version}-%{release}
Requires:       pkgconfig(libLogger)
Requires:       pkgconfig(libConfig)

%description -n libIpc-devel
The package provides libIpc development tools and libs.

%files -n libIpc-devel
%defattr(644,root,root,755)
%{_libdir}/libIpc.so
%{_includedir}/vasum-tools/ipc
%{_libdir}/pkgconfig/libIpc.pc
