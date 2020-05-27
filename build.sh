#!/bin/sh
LIBWEBRTC_INC=/Users/tobias/Developer/mediasoup/webrtc-checkout/src
LIBWEBRTC_BIN=/Users/tobias/Developer/mediasoup/webrtc-checkout/src/out/mybuild-m79/obj
cmake . -Bbuild                                              \
  -DLIBWEBRTC_INCLUDE_PATH:PATH=$LIBWEBRTC_INC \
  -DLIBWEBRTC_BINARY_PATH:PATH=$LIBWEBRTC_BIN   \
  -DOPENSSL_INCLUDE_DIR:PATH=/usr/local/Cellar/openssl@1.1/1.1.1g/include      \
  -DOPENSSL_ROOT_DIR:PATH=/usr/local/Cellar/openssl@1.1/1.1.1g \
  -DOPENSSL_LIBRARY_DIR:PATH=/usr/local/Cellar/openssl@1.1/1.1.1g/lib \
  -DCMAKE_USE_OPENSSL=ON
make -C build
