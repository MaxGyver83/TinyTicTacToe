#!/bin/sh
set -e

# export $KEYSTORE_PASSWORD and $KEYSTORE_ALIAS or change the lines below
# before running this script
KEYSTORE_PASSWORD="${KEYSTORE_PASSWORD:-12345678}"
KEYSTORE_ALIAS=${KEYSTORE_ALIAS:-playstore}

keytool -genkeypair \
    -dname "CN=Your Name, O=Your Organization, C=DE" \
    -alias $KEYSTORE_ALIAS \
    -keystore keystore.jks \
    -keypass "$KEYSTORE_PASSWORD" \
    -storepass "$KEYSTORE_PASSWORD" \
    -validity 10000 \
    -keyalg RSA
