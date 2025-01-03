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
$ANDROID_NDK_ROOT/ndk-build --directory=app/src/main APP_ABI="$ABI"
ret_code=$?
if [ $ret_code != 0 ]; then
    echo Error building native code!
    echo Error code: $ret_code
    exit $ret_code
fi

echo "Libraries created in app/src/main/libs"
tree --noreport app/src/main/libs


# echo Copy files from libs to a temporary folder
# mkdir -p lib
# cp -R app/src/main/libs/* lib/

# echo Delete temporary folder
# remove lib

# result:
# 54K Dec 29 23:14 ./lib/arm64-v8a/libtinytictactoe.so
# 54K Dec 31 11:22 ./build/lib/arm64-v8a/libtinytictactoe.so
# 54K Dec 29 23:14 ./app/src/main/libs/arm64-v8a/libtinytictactoe.so
# 54K Dec 29 23:14 ./app/src/main/obj/local/arm64-v8a/libtinytictactoe.so
# 3 idential, the 4th one differs
