#!/bin/sh
set -e
# Connect your Android device (adb connect) or start the Anrdoid emulator before
# running this script.

ANDROID_SDK_ROOT=$HOME/Android/Sdk
PATH="$ANDROID_SDK_ROOT/platform-tools:$PATH"

APP_NAME=TinyTicTacToe
ARCH="${1:-all}"
TAG=com.goodtemperapps.tinytictactoe

echo Clear logcat
adb logcat -c
echo Installing APK
adb install -r -d out/${APP_NAME}-${ARCH}.apk
echo Launching APK
adb shell am start -n $TAG/android.app.NativeActivity
