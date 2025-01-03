#!/bin/sh
set -e

AAB_FILE=out/TinyTicTacToe.aab
APKS_FILE=/tmp/TinyTicTacToe.apks
TAG=com.goodtemperapps.tinytictactoe

KEYSTORE_PASSWORD="${KEYSTORE_PASSWORD:-12345678}"
KEYSTORE_ALIAS=${KEYSTORE_ALIAS:-playstore}

test -f $APKS_FILE && rm $APKS_FILE
bundletool build-apks \
    --local-testing \
    --connected-device \
    --bundle=$AAB_FILE \
    --output=$APKS_FILE \
    --ks=keystore.jks \
    --ks-pass=pass:"$KEYSTORE_PASSWORD" \
    --ks-key-alias=$KEYSTORE_ALIAS \
    --key-pass=pass:"$KEYSTORE_PASSWORD"
# adb uninstall $TAG
adb logcat -c
bundletool install-apks --apks=$APKS_FILE --modules="base"
adb shell am start -n $TAG/android.app.NativeActivity
./adblog.sh
