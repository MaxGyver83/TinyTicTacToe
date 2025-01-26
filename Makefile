.PHONY: default x11 x11_clean apk install run log aab run_aab clean

APP_NAME = TinyTicTacToe
LIB_NAME = libtinytictactoe.so
# possible ABIs: all x86_64 arm64-v8a armeabi-v7a
ABI = all
APK_FILE = out/$(APP_NAME)-$(ABI).apk
AAB_FILE = out/$(APP_NAME).aab

default: run

x11:
	cd app/src/main/jni && make run
x11_clean:
	cd app/src/main/jni && make clean

keystore.jks:
	./create_keystore.sh
$(APK_FILE): keystore.jks
	./build_apk.sh $(ABI)
$(AAB_FILE): keystore.jks 
	./build_shared_library.sh all
	./build_aab.sh

apk: $(APK_FILE)
install: apk
	adb install -r -d $(APK_FILE)
run: apk
	./run.sh $(ABI)
	./adblog.sh
log:
	./adblog.sh

aab: $(AAB_FILE)
run_aab: aab
	./install_run_aab.sh
	./adblog.sh

clean:
	rm -rf out/
