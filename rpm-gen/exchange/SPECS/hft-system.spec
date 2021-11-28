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
BuildRequires:		make, centos-release-scl, devtoolset-7-gcc-c++, devtoolset-7-gcc, devtoolset-7-gcc-plugin-devel, devtoolset-7-gcc-gdb-plugin,openssl-devel, python-devel, curl-devel, sqlite-devel, newt-devel, maven
Requires:		curl, sqlite, newt, php-cli >= 5.4.16, java-1.8.0-openjdk
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
pushd bridges/dukascopy
cat pom.xml.in | sed "s/@HFTVERSION@/$(< ../../version)/g" > pom.xml
mvn clean install
popd
pushd etc/deploy
cat hft-config.xml.in | sed "s/@HFTVERSION@/$(< ../../version)/g" > hft-config.xml
popd

%install
rm -rf $RPM_BUILD_ROOT
# Install hft binary.
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
install -m 755 -p hft/hft $RPM_BUILD_ROOT%{_sbindir}/hft
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft
install -m 644 -p etc/deploy/hft-config.xml $RPM_BUILD_ROOT%{_sysconfdir}/hft/hft-config.xml
# Session-defaults data
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/AUDUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/COPPER.CMDUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/EUS.IDXEUR
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/HKG.IDXHKD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USA500.IDXUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDCNH
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDHUF
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDNOK
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDSEK
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDZAR
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/AUS.IDXAUD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/COTTON.CMDUSX
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/GAS.CMDUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/LIGHT.CMDUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDCAD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDDKK
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDJPY
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDPLN
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDSGD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/XAGUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/BRENT.CMDUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/EURUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/GBPUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/NZDUSD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDCHF
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDHKD
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDMXN
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDRUB
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDTRY
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/XAUUSD
install -m 644 -p etc/deploy/session-defaults/AUDUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/AUDUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/COPPER.CMDUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/COPPER.CMDUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/EUS.IDXEUR/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/EUS.IDXEUR/manifest.json
install -m 644 -p etc/deploy/session-defaults/HKG.IDXHKD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/HKG.IDXHKD/manifest.json
install -m 644 -p etc/deploy/session-defaults/USA500.IDXUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USA500.IDXUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDCNH/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDCNH/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDHUF/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDHUF/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDNOK/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDNOK/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDSEK/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDSEK/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDZAR/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDZAR/manifest.json
install -m 644 -p etc/deploy/session-defaults/AUS.IDXAUD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/AUS.IDXAUD/manifest.json
install -m 644 -p etc/deploy/session-defaults/COTTON.CMDUSX/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/COTTON.CMDUSX/manifest.json
install -m 644 -p etc/deploy/session-defaults/GAS.CMDUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/GAS.CMDUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/LIGHT.CMDUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/LIGHT.CMDUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDCAD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDCAD/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDDKK/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDDKK/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDJPY/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDJPY/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDPLN/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDPLN/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDSGD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDSGD/manifest.json
install -m 644 -p etc/deploy/session-defaults/XAGUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/XAGUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/BRENT.CMDUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/BRENT.CMDUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/EURUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/EURUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/GBPUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/GBPUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/NZDUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/NZDUSD/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDCHF/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDCHF/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDHKD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDHKD/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDMXN/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDMXN/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDRUB/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDRUB/manifest.json
install -m 644 -p etc/deploy/session-defaults/USDTRY/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/USDTRY/manifest.json
install -m 644 -p etc/deploy/session-defaults/XAUUSD/manifest.json $RPM_BUILD_ROOT%{_sysconfdir}/hft/session-defaults/XAUUSD/manifest.json
mkdir -p $RPM_BUILD_ROOT%{_unitdir}
install -m 644 -p etc/deploy/hft.service $RPM_BUILD_ROOT%{_unitdir}/hft.service
mkdir -p $RPM_BUILD_ROOT/%{_localstatedir}/log/hft
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/sessions
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways/dukascopy
# Install instrument handlers
install -m 755 -p instrument-handlers/smart_martingale/libsmart_martingale.so $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/instrument-handlers/
#Dukascopy bridge
install -m 644 -p bridges/dukascopy/target/hft-bridge-$(< version).jar $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways/dukascopy/hft-bridge-$(< version).jar
find /root/.m2/repository/ -name "*.jar" | while read line ; do install -m 644 -p  "$line" $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/marketplace-gateways/dukascopy/ ; done

%post
mkdir -p $RPM_BUILD_ROOT%{_sharedstatedir}/hft/{instrument-handlers,sessions,marketplace-gateways}
mkdir -p $RPM_BUILD_ROOT%{_sharedstatedir}/hft/marketplace-gateways/dukascopy
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
%{_sharedstatedir}/hft/marketplace-gateways/dukascopy/*.jar
%{_sysconfdir}/hft/session-defaults/AUDUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/COPPER.CMDUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/EUS.IDXEUR/manifest.json
%{_sysconfdir}/hft/session-defaults/HKG.IDXHKD/manifest.json
%{_sysconfdir}/hft/session-defaults/USA500.IDXUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/USDCNH/manifest.json
%{_sysconfdir}/hft/session-defaults/USDHUF/manifest.json
%{_sysconfdir}/hft/session-defaults/USDNOK/manifest.json
%{_sysconfdir}/hft/session-defaults/USDSEK/manifest.json
%{_sysconfdir}/hft/session-defaults/USDZAR/manifest.json
%{_sysconfdir}/hft/session-defaults/AUS.IDXAUD/manifest.json
%{_sysconfdir}/hft/session-defaults/COTTON.CMDUSX/manifest.json
%{_sysconfdir}/hft/session-defaults/GAS.CMDUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/LIGHT.CMDUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/USDCAD/manifest.json
%{_sysconfdir}/hft/session-defaults/USDDKK/manifest.json
%{_sysconfdir}/hft/session-defaults/USDJPY/manifest.json
%{_sysconfdir}/hft/session-defaults/USDPLN/manifest.json
%{_sysconfdir}/hft/session-defaults/USDSGD/manifest.json
%{_sysconfdir}/hft/session-defaults/XAGUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/BRENT.CMDUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/EURUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/GBPUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/NZDUSD/manifest.json
%{_sysconfdir}/hft/session-defaults/USDCHF/manifest.json
%{_sysconfdir}/hft/session-defaults/USDHKD/manifest.json
%{_sysconfdir}/hft/session-defaults/USDMXN/manifest.json
%{_sysconfdir}/hft/session-defaults/USDRUB/manifest.json
%{_sysconfdir}/hft/session-defaults/USDTRY/manifest.json
%{_sysconfdir}/hft/session-defaults/XAUUSD/manifest.json
%{_sharedstatedir}/hft/instrument-handlers/*.so*
%config(noreplace) %{_sysconfdir}/hft/hft-config.xml
