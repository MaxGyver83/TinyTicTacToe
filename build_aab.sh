#!/bin/sh
set -e

# wget https://dl.google.com/dl/android/maven2/com/android/tools/build/aapt2/8.7.3-12006047/aapt2-8.7.3-12006047-linux.jar
# jar xf aapt2-8.7.3-12006047-linux.jar
# mv aapt2 ~/.local/bin/

ANDROID_SDK_ROOT=$HOME/Android/Sdk
PATH=$ANDROID_SDK_ROOT/tools:$ANDROID_SDK_ROOT/platform-tools:$PATH
ANDROIDTARGET=35
ANDROID_JAR=$ANDROID_SDK_ROOT/platforms/android-$ANDROIDTARGET/android.jar

APP_NAME=TinyTicTacToe
AAB_FILE_UNSIGNED=out/${APP_NAME}-unsigned.aab
AAB_FILE=out/${APP_NAME}.aab

KEYSTORE_PASSWORD="${KEYSTORE_PASSWORD:-12345678}"
KEYSTORE_ALIAS=${KEYSTORE_ALIAS:-playstore}

[ -d build ] && rm -r build
[ -d compiled_resources ] && rm -r compiled_resources
[ -f resources.apk ] && rm resources.apk
[ -f $AAB_FILE_UNSIGNED ] && rm $AAB_FILE_UNSIGNED
[ -f $AAB_FILE ] && rm $AAB_FILE

echo "Compiling resources..."
mkdir -p compiled_resources
aapt2 compile \
    --dir app/src/main/res \
    -o compiled_resources/

echo "Linking resources..."
aapt2 link --proto-format -o resources.apk \
    -I "$ANDROID_JAR" \
    --manifest app/src/main/AndroidManifest.xml \
    -R compiled_resources/*.flat \
    --auto-add-overlay

echo "Extracting resources..."
unzip resources.apk -d build \
    && rm resources.apk
mkdir -p build/manifest build/root
mv build/AndroidManifest.xml build/manifest/
cp -r app/src/main/libs build/lib
cp -r app/src/main/assets build/

echo "Creating base.zip..."
cd build
jar cMf ../base.zip *
# same?: zip -r ../base.zip *
cd ..

echo "Building AAB..."
bundletool build-bundle --config=BundleConfig.json \
    --modules=base.zip --output=$AAB_FILE_UNSIGNED \
    && rm base.zip

echo "Signing AAB..."
jarsigner -keystore keystore.jks \
    -storepass "$KEYSTORE_PASSWORD" \
    -signedjar $AAB_FILE \
    $AAB_FILE_UNSIGNED $KEYSTORE_ALIAS

echo "$AAB_FILE created."
