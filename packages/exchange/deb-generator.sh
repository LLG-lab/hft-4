set -e

function error()
{
    local ERROR_MESSAGE="$1"

    if [ -t 1 ] ; then
        echo -e "\033[;31m ** ERROR! **\033[0m $ERROR_MESSAGE" >&2
    else
        echo "** ERROR! ** $ERROR_MESSAGE" >&2
    fi

    exit 1
}

function prep()
{
    ## Obtain HFT source code.
    mkdir /root/BUILD
    tar -xf /root/exchange/SOURCES/hft-project.tar -C /root/BUILD

    ## Attach precompiled boost libraries to the project tree.
    local DETECT_BOOST_DIR=$(pushd /root/ > /dev/null ; ls -d boost_* ; popd > /dev/null)

    if [ "x$DETECT_BOOST_DIR" = "x"  ] || [ ! -d "/root/$DETECT_BOOST_DIR" ] ; then
        error "Failed to autodetect boost directory ‘$DETECT_BOOST_DIR’"
    fi

    pushd /root/BUILD/3rd-party > /dev/null
        ln -vs /root/${DETECT_BOOST_DIR} boost_current_release
        ln -vs /root/${DETECT_BOOST_DIR}/boost boost
    popd > /dev/null
}

function post()
{
    local DEBPKG="${PACKAGE_NAME}.deb"
    echo "Exporting outcome ${DEBPKG}..."
    mkdir -p /root/exchange/DEBS/
    cp -a /root/$DEBPKG /root/exchange/DEBS/
}

function get_build_number()
{
    is_natural_number() {
        [[ ${1} =~ ^[0-9]+$ ]]
    }

    local BUILD_UID=$(< /root/exchange/build.uid)

    if ! is_natural_number $BUILD_UID ; then
        error "Data ‘${BUILD_UID}’ is not a number. Fix file exchange/build.uid"
    fi

    BUILD_UID=$((BUILD_UID+1))

    echo $BUILD_UID > /root/exchange/build.uid
    echo $BUILD_UID
}

function get_version()
{
    local V="${VERSION_MAJOR}.${VERSION_MINOR}"

    if [ $BUILD_NUMBER -gt 0 ] ; then
        V="${V}-${BUILD_NUMBER}"
    fi

    echo $V
}

function get_package_name()
{
    local V=$(get_version)
    local PNAME="hft-system_${V}_amd64"
    echo $PNAME
}

function make_pkg_target()
{
    local DIRECTORIES="
      /root/${PACKAGE_NAME}/DEBIAN
      /root/${PACKAGE_NAME}/etc/hft
      /root/${PACKAGE_NAME}/usr/sbin
      /root/${PACKAGE_NAME}/lib/systemd/system
      /root/${PACKAGE_NAME}/var/log/hft
      /root/${PACKAGE_NAME}/var/lib/hft/sessions
      /root/${PACKAGE_NAME}/var/lib/hft/instrument-handlers
      /root/${PACKAGE_NAME}/var/lib/hft/marketplace-gateways/ctrader
    "

    for d in $DIRECTORIES ; do
        mkdir -v -m 0755 -p $d
    done

    ## Copy unit file
    cp /root/BUILD/etc/deploy/hft.service /root/${PACKAGE_NAME}/lib/systemd/system/
    chmod 0644 /root/${PACKAGE_NAME}/lib/systemd/system/hft.service

    ## Copy configuration file
    cp /root/BUILD/etc/deploy/hft-config.xml /root/${PACKAGE_NAME}/etc/hft/
    chmod 0644 /root/${PACKAGE_NAME}/etc/hft/hft-config.xml

    ## Don't override configuration file on package update
    echo "/etc/hft/hft-config.xml" > /root/${PACKAGE_NAME}/DEBIAN/conffiles
}

function get_depends()
{
    pushd $(mktemp -d) > /dev/null
    cp -a /root/BUILD/hft/hft .
    cp -a /root/BUILD/bridges/ctrader/hft2ctrader .

    local BINARIES=$(ls)

    mkdir debian
    touch debian/control

    local X=$(dpkg-shlibdeps -O $BINARIES)
    local DEPENDS=${X:15}
    local VERIFY=${X::15}

    popd > /dev/null

    if [ "x$VERIFY" != "xshlibs:Depends=" ] ; then
        error "Something went wrong with get library dependencies: ${X}"
    fi

    echo $DEPENDS
}

function create_control_file()
{
    local HFT_VERSION=$(get_version)
    local DEPS=$(get_depends)

cat << EOF > /root/${PACKAGE_NAME}/DEBIAN/control
Package: hft
Version: ${HFT_VERSION}
Architecture: amd64
Maintainer: LLG Ryszard Gradowski <ryszard.gradowski@gmail.com>
Depends: ${DEPS}
Provides: hft
Section: net
Priority: optional
Description: High Frequency Trading System - Professional Expert Advisor
 This is a core part of High Frequency Trading System.
 Package delivers HFT server and HFT-related toolset.
EOF

    chmod 0644 /root/${PACKAGE_NAME}/DEBIAN/control
}

function build_and_install_hft()
{
    pushd /root/BUILD/hft > /dev/null
    ./configure --release
    make
    strip hft
    cp hft /root/${PACKAGE_NAME}/usr/sbin/
    chmod 0755 /root/${PACKAGE_NAME}/usr/sbin/hft
    popd > /dev/null
}

function build_and_install_hft2ctrader()
{
    pushd /root/BUILD/bridges/ctrader > /dev/null
    ./configure --release
    make
    strip hft2ctrader
    cp hft2ctrader /root/${PACKAGE_NAME}/var/lib/hft/marketplace-gateways/ctrader/
    chmod 0755 /root/${PACKAGE_NAME}/var/lib/hft/marketplace-gateways/ctrader/hft2ctrader
    popd > /dev/null
}

function build_and_install_instrument_handler()
{
    local IH=$1

    pushd /root/BUILD/instrument-handlers/$IH > /dev/null
    cmake . -DCMAKE_BUILD_TYPE:STRING=Release
    make
    strip lib${IH}.so
    install -m 644 -p lib${IH}.so /root/${PACKAGE_NAME}/var/lib/hft/instrument-handlers/

    popd > /dev/null
}


##############
## Main Exec
##############

declare -r BUILD_NUMBER=$(get_build_number)

[ "x$BUILD_NUMBER" = "x" ] && exit 1

prep

declare -r VERSION_MAJOR=$(/root/BUILD/etc/version.sh --major)
declare -r VERSION_MINOR=$(/root/BUILD/etc/version.sh --minor)
declare -r PACKAGE_NAME=$(get_package_name)

echo "Called internal deb-generator, Build version ${VERSION_MAJOR}.${VERSION_MINOR}, UID ‘${BUILD_NUMBER}’"

pushd /root/ > /dev/null

make_pkg_target
build_and_install_hft
build_and_install_hft2ctrader
build_and_install_instrument_handler smart_martingale
build_and_install_instrument_handler simple_tracker
build_and_install_instrument_handler blsh
build_and_install_instrument_handler grid
build_and_install_instrument_handler xgrid
create_control_file

dpkg-deb --build --root-owner-group $PACKAGE_NAME

popd > /dev/null

post

exit 0
