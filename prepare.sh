#!/bin/sh

mkdir -p thirdparty
cd thirdparty

## Install depot tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PWD/depot_tools:$PATH

## Checkout webrtc
mkdir webrtc-checkout
cd webrtc-checkout
fetch --nohooks webrtc
gclient sync
cd src
git checkout -b m84 refs/remotes/branch-heads/4147
gclient sync

## Build webrtc
gn gen out/m84 --args='is_debug=false is_component_build=false is_clang=true rtc_include_tests=true rtc_use_h264=true rtc_enable_protobuf=false use_rtti=true mac_deployment_target="10.11" use_custom_libcxx=false'
ninja -C out/m84
LIBWEBRTC_INC=$PWD
LIBWEBRTC_BIN=$PWD/out/m84/obj
cd ..
cd ..
cd ..

# Shall we install brew?
which -s brew
if [[ $? != 0 ]] ; then
    # Install Homebrew
    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi
brew install openssl

OPENSSL_INCLUDE_DIR=/usr/local/opt/openssl/include
OPENSSL_ROOT_DIR=/usr/local/opt/openssl
OPENSSL_LIBRARY_DIR=/usr/local/opt/openssl/lib

npm install

npm config set cmake_LIBWEBRTC_INCLUDE_PATH $LIBWEBRTC_INC
npm config set cmake_LIBWEBRTC_BINARY_PATH $LIBWEBRTC_BIN
npm config set cmake_OPENSSL_INCLUDE_DIR $OPENSSL_INCLUDE_DIR
npm config set cmake_OPENSSL_ROOT_DIR $OPENSSL_ROOT_DIR
npm config set cmake_OPENSSL_LIBRARY_DIR $OPENSSL_LIBRARY_DIR
npm config set cmake_CMAKE_USE_OPENSSL ON

npm run make

npm run start
