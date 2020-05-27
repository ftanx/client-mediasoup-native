#!/bin/sh
LIBWEBRTC_INCLUDE_PATH=/Users/tobias/Developer/mediasoup/webrtc-checkout/src
LIBWEBRTC_BINARY_PATH=/Users/tobias/Developer/mediasoup/webrtc-checkout/src/out/mybuild-m79/obj
OPENSSL_INCLUDE_DIR=/usr/local/Cellar/openssl@1.1/1.1.1g/include
OPENSSL_ROOT_DIR=/usr/local/Cellar/openssl@1.1/1.1.1g
OPENSSL_LIBRARY_DIR=/usr/local/Cellar/openssl@1.1/1.1.1g/lib
npm config set cmake_LIBWEBRTC_INCLUDE_PATH $LIBWEBRTC_INCLUDE_PATH
npm config set cmake_LIBWEBRTC_BINARY_PATH $LIBWEBRTC_BINARY_PATH
npm config set cmake_OPENSSL_INCLUDE_DIR $OPENSSL_INCLUDE_DIR
npm config set cmake_OPENSSL_ROOT_DIR $OPENSSL_ROOT_DIR
npm config set cmake_OPENSSL_LIBRARY_DIR $OPENSSL_LIBRARY_DIR
npm config set cmake_CMAKE_USE_OPENSSL ON
npm run compile

#cmake . -Bbuild                                              \
#  -DLIBWEBRTC_INCLUDE_PATH:PATH=Users/tobias/Developer/mediasoup/webrtc-checkout/src \
#  -DLIBWEBRTC_BINARY_PATH:PATH=/Users/tobias/Developer/mediasoup/webrtc-checkout/src/out/mybuild-m79/obj   \
#  -DOPENSSL_INCLUDE_DIR:PATH=/usr/local/Cellar/openssl@1.1/1.1.1g/include      \
#  -DOPENSSL_ROOT_DIR:PATH=/usr/local/Cellar/openssl@1.1/1.1.1g \
#  -DOPENSSL_LIBRARY_DIR:PATH=/usr/local/Cellar/openssl@1.1/1.1.1g/lib \
#  -DCMAKE_USE_OPENSSL=ON
#make -C build
