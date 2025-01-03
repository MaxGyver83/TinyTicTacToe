#!/bin/sh
TAG=com.goodtemperapps.tinytictactoe
# adb -d logcat -s -v color --pid=$(adb -d shell pidof -s com.goodtemperapps.tinytictactoe)
adb logcat -v color -s $TAG:D 2>&1 \
    | sed -E -e 's/..-.. //' \
        -e 's/[0-9]+ [0-9]+ //' \
        -e "s/ $TAG//"
