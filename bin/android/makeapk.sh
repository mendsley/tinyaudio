#!/bin/bash

set -e

if [[ -z $ANDROID_SDK ]]; then
	echo "Must set the ANDROID_SDK variable" >&2
	exit 1
fi

if [ ! -d obj ]; then
	mkdir obj
fi

javac -verbose \
		-d obj \
		-classpath $ANDROID_SDK/platforms/android-14/android.jar:obj \
		-sourcepath src \
		src/com/github/mendsley/tinyaudio/*.java

$ANDROID_SDK/platform-tools/dx --dex \
		--verbose \
		--output=bin/classes.dex \
		obj \
		bin/lib

$ANDROID_SDK/platform-tools/aapt package --debug-mode -v -f \
		-M AndroidManifest.xml \
		-S res \
		-I $ANDROID_SDK/platforms/android-14/android.jar \
		-F TinyAudio.unsigned.apk \
		bin

jarsigner -verbose \
		-keystore keystore \
		-storepass tinyaudio \
		-keypass tinyaudio \
		-signedjar TinyAudio.signed.apk \
		TinyAudio.unsigned.apk \
		android
rm -r TinyAudio.unsigned.apk

$ANDROID_SDK/tools/zipalign -v -f 4 TinyAudio.signed.apk TinyAudio.apk
rm -f TinyAudio.signed.apk
