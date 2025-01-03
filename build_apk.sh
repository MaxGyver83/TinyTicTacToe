#!/bin/sh
set -e

usage() {
    echo "Usage: $0 x86_64|arm64-v8a|armeabi-v7a|all"
}

export ANDROID_SDK_ROOT=$HOME/Android/Sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/27.2.12479018
export PATH=$ANDROID_SDK_ROOT/tools:$ANDROID_SDK_ROOT/platform-tools:$PATH

export ANDROIDTARGET=35
export BUILDTOOLSVERSION=35.0.0
BUILDTOOLSDIR=$ANDROID_SDK_ROOT/build-tools/$BUILDTOOLSVERSION
ANDROID_JAR=$ANDROID_SDK_ROOT/platforms/android-$ANDROIDTARGET/android.jar

KEYSTORE_PASSWORD="${KEYSTORE_PASSWORD:-12345678}"
KEYSTORE_ALIAS=${KEYSTORE_ALIAS:-playstore}

case "$1" in
    -h|--help) usage; exit 0 ;;
    x86_64|arm64-v8a|armeabi-v7a)
          ABI=$1; APKNAME=TinyTicTacToe-$ABI ;;
    x86)  ABI=x86_64; APKNAME=TinyTicTacToe-$ABI  ;;
    arm8) ABI=arm64-v8a; APKNAME=TinyTicTacToe-$ABI  ;;
    arm7) ABI=armeabi-v7a; APKNAME=TinyTicTacToe-$ABI ;;
    all)  ABI="x86_64 arm64-v8a armeabi-v7a"; APKNAME=TinyTicTacToe-all ;;
    *)    usage; exit 1 ;;
esac

remove() {
    if [ -d "$1" ]; then
        echo Deleting "$1"
        rm -r "$1"
    fi
}

./build_shared_library.sh "$1" || exit 1

remove lib

echo Creating empty APK...
mkdir -p app/build/outputs/apk
$BUILDTOOLSDIR/aapt package -f \
    -M app/src/main/AndroidManifest.xml \
    -S app/src/main/res \
    -A app/src/main/assets \
    -I "$ANDROID_JAR" \
    -F app/build/outputs/apk/unaligned.apk
ret_code=$?
if [ $ret_code != 0 ]; then
    echo Error creating empty APK!
    echo Error code: $ret_code
    exit $ret_code
fi

echo Copy files from libs to a temporary folder
mkdir -p lib
for arch in $ABI; do
    cp -R app/src/main/libs/$arch lib/
done

echo Add the contents of the temporary folder to the archive in the lib folder
zip app/build/outputs/apk/unaligned.apk -r lib/* lib/

echo Aligning APK...
$BUILDTOOLSDIR/zipalign -f 4 app/build/outputs/apk/unaligned.apk app/build/outputs/apk/$APKNAME-unsigned.apk
ret_code=$?
if [ $ret_code != 0 ]; then
    echo Error aligning APK!
    echo Error code: $ret_code
    exit $ret_code
fi

echo Signing APK...
$BUILDTOOLSDIR/apksigner sign \
    --ks keystore.jks \
    --ks-key-alias $KEYSTORE_ALIAS \
    --ks-pass pass:"$KEYSTORE_PASSWORD" \
    --out app/build/outputs/apk/$APKNAME.apk \
    app/build/outputs/apk/$APKNAME-unsigned.apk
ret_code=$?
if [ $ret_code != 0 ]; then
    echo Error signing APK!
    echo Error code: $ret_code
    exit $ret_code
fi

echo Delete temporary folder
remove lib/

echo Deleting unnecessary files...
rm app/build/outputs/apk/$APKNAME-unsigned.apk
rm app/build/outputs/apk/$APKNAME.apk.idsig
rm app/build/outputs/apk/unaligned.apk

mkdir -p out
cp app/build/outputs/apk/$APKNAME.apk out/$APKNAME.apk

echo APK successfully created: out/$APKNAME.apk
