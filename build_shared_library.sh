#!/bin/sh
set -e

usage() {
    echo "Usage: $0 x86_64|arm64-v8a|armeabi-v7a|all"
}

ANDROID_SDK_ROOT=$HOME/Android/Sdk
ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/27.2.12479018

case "$1" in
    -h|--help) usage; exit 0 ;;
    x86_64|arm64-v8a|armeabi-v7a)
               ABI=$1 ;;
    x86)       ABI=x86_64 ;;
    arm8)      ABI=arm64-v8a ;;
    arm7)      ABI=armeabi-v7a;;
    all)       ABI="x86_64 arm64-v8a armeabi-v7a" ;;
    *)         usage; exit 1;;
esac

remove() {
    if [ -d "$1" ]; then
        echo Deleting "$1"
        rm -r "$1"
    fi
}

echo Cleaning previous builds...
remove app/build
for arch in $ABI; do
    remove app/src/main/libs/$arch
    remove app/src/main/obj/local/$arch
done

echo Building native code for $ABI...
# ndk-build creates unstripped libs in app/src/main/obj/local/$ABI
# and stripped libs in libs/$ABI (for DEBUG=1 these remain unstripped)
$ANDROID_NDK_ROOT/ndk-build ${DEBUG:+NDK_DEBUG=1} --directory=app/src/main APP_ABI="$ABI"
ret_code=$?
if [ $ret_code != 0 ]; then
    echo Error building native code!
    echo Error code: $ret_code
    exit $ret_code
fi

echo "Libraries created in app/src/main/libs"
tree --noreport app/src/main/libs
