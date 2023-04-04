%define name		hft-system
%define rel		0_%{build_number}
%define version		%{hft_version_major}.%{hft_version_minor}
%global __os_install_post %{nil}

BuildRoot:		%{buildroot}
Summary:		High Frequency Trading System - Professional Expert Advisor
License:		Propertiary
Name:			%{name}
Version:		%{version}
Release:		%{rel}.el%{?rhel}
Group:			System Environment/Daemons
Vendor:			LLG Ryszard Gradowski
Packager:		LLG Ryszard Gradowski
BuildRequires:		make, centos-release-scl, devtoolset-7-gcc-c++, devtoolset-7-gcc, devtoolset-7-gcc-plugin-devel, devtoolset-7-gcc-gdb-plugin,openssl-devel, python-devel, curl-devel, protobuf-devel = 2.5.0, protobuf-compiler = 2.5.0
Requires:		curl, protobuf = 2.5.0
Requires(preun):	systemd-units
Requires(postun):	systemd-units
Requires(post):		systemd-units

%description
This is a core part of High Frequency Trading System.
Package delivers HFT server and HFT-related toolset.

%prep
# We have source already prepared on
# toplevel script, so nothing here.

%build
pushd hft
scl enable devtoolset-7 "./configure --release"
scl enable devtoolset-7 make
popd
pushd instrument-handlers/smart_martingale
scl enable devtoolset-7 "cmake . -DCMAKE_BUILD_TYPE:STRING=Release"
scl enable devtoolset-7 make
popd
pushd instrument-handlers/simple_tracker
scl enable devtoolset-7 "cmake . -DCMAKE_BUILD_TYPE:STRING=Release"
scl enable devtoolset-7 make
popd
pushd instrument-handlers/blsh
scl enable devtoolset-7 "cmake . -DCMAKE_BUILD_TYPE:STRING=Release"
scl enable devtoolset-7 make
popd
pushd instrument-handlers/grid
scl enable devtoolset-7 "cmake . -DCMAKE_BUILD_TYPE:STRING=Release"
scl enable devtoolset-7 make
popd
pushd instrument-handlers/xgrid
scl enable devtoolset-7 "cmake . -DCMAKE_BUILD_TYPE:STRING=Release"
scl enable devtoolset-7 make
popd
pushd  bridges/ctrader
scl enable devtoolset-7 "./configure --release"
scl enable devtoolset-7 make
popd

%install
rm -rf $RPM_BUILD_ROOT
# Install hft binary.
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
install -m 755 -p hft/hft $RPM_BUILD_ROOT%{_sbindir}/hft
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft
install -m 644 -p etc/deploy/hft-config.xml $RPM_BUILD_ROOT%{_sysconfdir}/hft/hft-config.xml
mkdir -p $RPM_BUILD_ROOT%{_unitdir}
install -m 644 -p etc/deploy/hft.service $RPM_BUILD_ROOT%{_unitdir}/hft.service
mkdir -p $RPM_BUILD_ROOT/%{_localstatedir}/log/hft
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/sessions
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways/ctrader
# Install instrument handlers
install -m 755 -p instrument-handlers/smart_martingale/libsmart_martingale.so $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers/
install -m 755 -p instrument-handlers/simple_tracker/libsimple_tracker.so $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers/
install -m 755 -p instrument-handlers/blsh/libblsh.so $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers/
install -m 755 -p instrument-handlers/grid/libgrid.so $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers/
install -m 755 -p instrument-handlers/xgrid/libxgrid.so $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers/
# Install cTrader bridge
install -m 755 -p bridges/ctrader/hft2ctrader $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways/ctrader/hft2ctrader

%post
mkdir -p $RPM_BUILD_ROOT%{_sharedstatedir}/hft/{instrument-handlers,sessions,marketplace-gateways}
mkdir -p $RPM_BUILD_ROOT%{_sharedstatedir}/hft/marketplace-gateways/dukascopy
mkdir -p $RPM_BUILD_ROOT%{_sharedstatedir}/hft/marketplace-gateways/ctrader
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/log/hft
%systemd_post hft.service

%preun
%systemd_preun hft.service

%postun
%systemd_postun_with_restart hft.service

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_sbindir}/hft
%{_unitdir}/hft.service
%{_sharedstatedir}/hft/marketplace-gateways/ctrader/hft2ctrader
%{_sharedstatedir}/hft/instrument-handlers/*.so*
%config(noreplace) %{_sysconfdir}/hft/hft-config.xml
