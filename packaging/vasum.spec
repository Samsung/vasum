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
# Default build with dbus
%{!?without_dbus:%define without_dbus 0}
# Default build with systemd
%{!?without_systemd:%define without_systemd 0}

Name:           vasum
Epoch:          1
Version:        0.1.2
Release:        0
Source0:        %{name}-%{version}.tar.gz
License:        Apache-2.0
Group:          Security/Other
Summary:        Daemon for managing zones
BuildRequires:  cmake
BuildRequires:  boost-devel
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  lxc-devel
BuildRequires:  readline-devel
BuildRequires:  pkgconfig(sqlite3)
Requires:       lxc
%if !%{without_systemd}
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(libsystemd-journal)
%endif
%if %{platform_type} == "TIZEN"
BuildRequires:  libjson-devel >= 0.10
Requires:       iproute2
Requires(post): libcap-tools
%else
BuildRequires:  json-c-devel
Requires:       lxc-templates
Requires:       iproute
Requires(post): libcap
%endif
Conflicts:      vasum-daemon < 1:0

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
%attr(755,root,root) %{_libexecdir}/vasum-launcher
%dir /etc/vasum
%dir /etc/vasum/templates
%config /etc/vasum/daemon.conf
%attr(755,root,root) /etc/vasum/templates/*.sh
%config /etc/vasum/templates/*.conf
%if !%{without_systemd}
%{_unitdir}/vasum.service
%{_unitdir}/vasum.socket
%{_unitdir}/multi-user.target.wants/vasum.service
%endif
%if !%{without_dbus}
%config /etc/dbus-1/system.d/org.tizen.vasum.host.conf
%endif
%dir %{_datadir}/zones

%prep
%setup -q

%build
%{!?build_type:%define build_type "RELEASE"}

%if %{build_type} == "DEBUG" || %{build_type} == "PROFILING" || %{build_type} == "CCOV"
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
         -DWITHOUT_DBUS=%{?without_dbus} \
         -DWITHOUT_SYSTEMD=%{?without_systemd}
make -k %{?jobs:-j%jobs}

%install
%make_install
%if !%{without_systemd}
mkdir -p %{buildroot}/%{_unitdir}/multi-user.target.wants
ln -s ../vasum.service %{buildroot}/%{_unitdir}/multi-user.target.wants/vasum.service
%endif
mkdir -p %{buildroot}/%{_datadir}/zones
%if %{platform_type} == "TIZEN"
ln -s tizen.conf %{buildroot}/etc/vasum/templates/default.conf
%else
ln -s fedora-minimal.conf %{buildroot}/etc/vasum/templates/default.conf
%endif

%clean
rm -rf %{buildroot}

%post
%if !%{without_systemd}
# Refresh systemd services list after installation
if [ $1 == 1 ]; then
    systemctl daemon-reload || :
fi
%endif

# set needed caps on the binary to allow restart without loosing them
setcap CAP_SYS_ADMIN,CAP_MAC_OVERRIDE,CAP_SYS_TTY_CONFIG+ei %{_bindir}/vasum-server

%preun
%if !%{without_systemd}
# Stop the service before uninstall
if [ $1 == 0 ]; then
     systemctl stop vasum.service || :
fi
%endif

%postun
%if !%{without_systemd}
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
%endif

## Client Package ##############################################################
%package client
Summary:          Vasum Client
Group:            Development/Libraries
Requires:         vasum = %{epoch}:%{version}-%{release}
Conflicts:        vasum < 1:0
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
%{_includedir}/vasum
%{_libdir}/pkgconfig/vasum-client.pc

%if !%{without_dbus}
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
%endif

## Command Line Interface ######################################################
%package cli
Summary:          Vasum Command Line Interface
Group:            Security/Other
Requires:         vasum-client = %{epoch}:%{version}-%{release}

%description cli
Command Line Interface for vasum.

%files cli
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vsm

%package cli-completion
Summary:          Vasum Command Line Interface bash completion
Group:            Security/Other
Requires:         vasum-cli = %{epoch}:%{version}-%{release}
#Requires:         bash-completion

%description cli-completion
Command Line Interface bash completion for vasum.

%files cli-completion
%attr(755,root,root) %{_sysconfdir}/bash_completion.d/vsm-completion.sh

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
%if !%{without_systemd}
systemctl daemon-reload || :
systemctl enable vasum-socket-test.socket || :
systemctl start vasum-socket-test.socket || :
%endif

%preun tests
%if !%{without_systemd}
systemctl stop vasum-socket-test.socket || :
systemctl disable vasum-socket-test.socket || :
%endif

%postun tests
%if !%{without_systemd}
systemctl daemon-reload || :
%endif

%files tests
%if %{platform_type} == "TIZEN"
%manifest packaging/vasum-server-tests.manifest
%endif
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/vasum-server-unit-tests
%if !%{without_systemd}
%attr(755,root,root) %{_bindir}/vasum-socket-test
%endif
%attr(755,root,root) %{script_dir}/vsm_all_tests.py
%attr(755,root,root) %{script_dir}/vsm_int_tests.py
%attr(755,root,root) %{script_dir}/vsm_launch_test.py
%{script_dir}/vsm_test_parser.py
%config /etc/vasum/tests/*.conf
%if !%{without_dbus}
%config /etc/vasum/tests/dbus/*.conf
%config /etc/dbus-1/system.d/org.tizen.vasum.tests.conf
%endif
%config /etc/vasum/tests/provision/*.conf
%config /etc/vasum/tests/templates/*.conf
%attr(755,root,root) /etc/vasum/tests/templates/*.sh
%dir /etc/vasum/tests/utils
%config /etc/vasum/tests/utils/*.txt
%attr(755,root,root) /etc/vasum/tests/utils/*.sh
%{python_sitelib}/vsm_integration_tests
%if !%{without_systemd}
%{_unitdir}/vasum-socket-test.socket
%{_unitdir}/vasum-socket-test.service
%endif

## libLogger Package ###########################################################
%package -n libLogger
Summary:            Logger library
Group:              Security/Other
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

%if !%{without_dbus}
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
%endif

## libcargo-devel Package ##########################################################
%package -n libcargo-devel
Summary:        Development C++ object serialization library
Group:          Development/Libraries
Requires:       boost-devel
Requires:       pkgconfig(libLogger)

%description -n libcargo-devel
The package provides libcargo development tools and libs.

%files -n libcargo-devel
%defattr(644,root,root,755)
%{_includedir}/cargo
%{_libdir}/pkgconfig/libcargo.pc

## libcargo-gvariant Package ##################################################
%package -n libcargo-gvariant-devel
Summary:        Development cargo GVariant module
Group:          Development/Libraries
Requires:       libcargo-devel = %{epoch}:%{version}-%{release}
Requires:       boost-devel
Requires:       pkgconfig(libLogger)

%description -n libcargo-gvariant-devel
The package provides libcargo GVariant development module.

%files -n libcargo-gvariant-devel
%defattr(644,root,root,755)
%{_includedir}/cargo-gvariant
%{_libdir}/pkgconfig/libcargo-gvariant.pc

## libcargo-json Package ######################################################
%package -n libcargo-json
Summary:            Cargo Json module
Group:              Security/Other
%if %{platform_type} == "TIZEN"
Requires:           libjson >= 0.10
%else
Requires:           json-c
%endif
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libcargo-json
The package provides libcargo Json module.

%post -n libcargo-json -p /sbin/ldconfig

%postun -n libcargo-json -p /sbin/ldconfig

%files -n libcargo-json
%defattr(644,root,root,755)
%{_libdir}/libcargo-json.so.0
%attr(755,root,root) %{_libdir}/libcargo-json.so.%{version}

%package -n libcargo-json-devel
Summary:        Development cargo Json module
Group:          Development/Libraries
Requires:       libcargo-devel = %{epoch}:%{version}-%{release}
Requires:       libcargo-json = %{epoch}:%{version}-%{release}
Requires:       boost-devel
Requires:       pkgconfig(libLogger)
%if %{platform_type} == "TIZEN"
Requires:       libjson-devel >= 0.10
%else
Requires:       json-c-devel
%endif

%description -n libcargo-json-devel
The package provides libcargo Json development module.

%files -n libcargo-json-devel
%defattr(644,root,root,755)
%{_libdir}/libcargo-json.so
%{_includedir}/cargo-json
%{_libdir}/pkgconfig/libcargo-json.pc

## libcargo-sqlite Package ##########################################################
%package -n libcargo-sqlite
Summary:            Cargo SQLite KVStore module
Group:              Security/Other
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libcargo-sqlite
The package provides libcargo SQLite KVStore library.

%post -n libcargo-sqlite -p /sbin/ldconfig

%postun -n libcargo-sqlite -p /sbin/ldconfig

%files -n libcargo-sqlite
%defattr(644,root,root,755)
%{_libdir}/libcargo-sqlite.so.0
%attr(755,root,root) %{_libdir}/libcargo-sqlite.so.%{version}

%package -n libcargo-sqlite-devel
Summary:        Cargo SQLite KVStore development module
Group:          Development/Libraries
Requires:       libcargo-devel = %{epoch}:%{version}-%{release}
Requires:       boost-devel
Requires:       pkgconfig(libLogger)

%description -n libcargo-sqlite-devel
The package provides libcargo SQLite KVStore development module.

%files -n libcargo-sqlite-devel
%defattr(644,root,root,755)
%{_libdir}/libcargo-sqlite.so
%{_includedir}/cargo-sqlite
%{_libdir}/pkgconfig/libcargo-sqlite.pc

## libcargo-fd Package ##########################################################
%package -n libcargo-fd
Summary:            Cargo file descriptor I/O module
Group:              Security/Other
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libcargo-fd
The package provides libcargo file descriptor I/O module.

%post -n libcargo-fd -p /sbin/ldconfig

%postun -n libcargo-fd -p /sbin/ldconfig

%files -n libcargo-fd
%defattr(644,root,root,755)
%{_libdir}/libcargo-fd.so.0
%attr(755,root,root) %{_libdir}/libcargo-fd.so.%{version}

%package -n libcargo-fd-devel
Summary:        Development C++ object serialization library
Group:          Development/Libraries
Requires:       libcargo-devel = %{epoch}:%{version}-%{release}
Requires:       boost-devel
Requires:       pkgconfig(libLogger)

%description -n libcargo-fd-devel
The package provides libcargo file descriptor I/O module.

%files -n libcargo-fd-devel
%defattr(644,root,root,755)
%{_libdir}/libcargo-fd.so
%{_includedir}/cargo-fd
%{_libdir}/pkgconfig/libcargo-fd.pc

## libcargo-sqlite-json Package ##########################################################
%package -n libcargo-sqlite-json-devel
Summary:        Cargo SQLite with Json defaults development module
Group:          Development/Libraries
Requires:       libcargo-sqlite-devel = %{epoch}:%{version}-%{release}
Requires:       libcargo-json-devel = %{epoch}:%{version}-%{release}
Requires:       boost-devel
Requires:       pkgconfig(libLogger)

%description -n libcargo-sqlite-json-devel
The package provides libcargo SQLite with Json defaults development module.

%files -n libcargo-sqlite-json-devel
%defattr(644,root,root,755)
%{_includedir}/cargo-sqlite-json
%{_libdir}/pkgconfig/libcargo-sqlite-json.pc

## libcargo-ipc Package #######################################################
%package -n libcargo-ipc
Summary:            Cargo IPC library
Group:              Security/Other
Requires:           libcargo-fd
Requires:           libuuid
BuildRequires:      libuuid-devel
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libcargo-ipc
The package provides libcargo-ipc library.

%post -n libcargo-ipc -p /sbin/ldconfig

%postun -n libcargo-ipc -p /sbin/ldconfig

%files -n libcargo-ipc
%defattr(644,root,root,755)
%{_libdir}/libcargo-ipc.so.0
%attr(755,root,root) %{_libdir}/libcargo-ipc.so.%{version}

%package -n libcargo-ipc-devel
Summary:        Development cargo IPC library
Group:          Development/Libraries
Requires:       libcargo-ipc = %{epoch}:%{version}-%{release}
Requires:       pkgconfig(libLogger)
Requires:       pkgconfig(libcargo)

%description -n libcargo-ipc-devel
The package provides libcargo-ipc development tools and libs.

%files -n libcargo-ipc-devel
%defattr(644,root,root,755)
%{_libdir}/libcargo-ipc.so
%{_includedir}/cargo-ipc
%{_libdir}/pkgconfig/libcargo-ipc.pc

## libcargo-validator Package #######################################################
%package -n libcargo-validator
Summary:            Cargo Validator library
Group:              Security/Other
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n libcargo-validator
The package provides libcargo-validator library.

%post -n libcargo-validator -p /sbin/ldconfig

%postun -n libcargo-validator -p /sbin/ldconfig

%files -n libcargo-validator
%defattr(644,root,root,755)
%{_libdir}/libcargo-validator.so.0
%{_libdir}/pkgconfig/libcargo-validator.pc
%attr(755,root,root) %{_libdir}/libcargo-validator.so.%{version}

%package -n libcargo-validator-devel
Summary:        Development Cargo Validator library
Group:          Development/Libraries
Requires:       libcargo-validator = %{epoch}:%{version}-%{release}
Requires:       pkgconfig(libLogger)
Requires:       pkgconfig(libcargo)

%description -n libcargo-validator-devel
The package provides libcargo-validator development tools and libs.

%files -n libcargo-validator-devel
%defattr(644,root,root,755)
%{_libdir}/libcargo-validator.so
%{_includedir}/cargo-validator
%{_libdir}/pkgconfig/libcargo-validator.pc

## liblxcpp Package ###########################################################
%package -n liblxcpp
Summary:            lxcpp library
Group:              Security/Other
Requires(post):     /sbin/ldconfig
Requires(postun):   /sbin/ldconfig

%description -n liblxcpp
The package provides liblxcpp library.

%post -n liblxcpp -p /sbin/ldconfig

%postun -n liblxcpp -p /sbin/ldconfig

%files -n liblxcpp
%defattr(644,root,root,755)
%{_libdir}/liblxcpp.so.0
%attr(755,root,root) %{_libexecdir}/lxcpp-guard
%attr(755,root,root) %{_libexecdir}/lxcpp-attach
%attr(755,root,root) %{_libdir}/liblxcpp.so.%{version}

%package -n liblxcpp-devel
Summary:        Development lxcpp library
Group:          Development/Libraries
Requires:       liblxcpp = %{epoch}:%{version}-%{release}

%description -n liblxcpp-devel
The package provides liblxcpp development tools and libs.

%files -n liblxcpp-devel
%defattr(644,root,root,755)
%{_libdir}/liblxcpp.so
%{_includedir}/lxcpp
%{_libdir}/pkgconfig/liblxcpp.pc
