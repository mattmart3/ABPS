#!/usr/bin/env bash

usage="usage: $0 [android|linux]"

if [ $# -le 0 ] ; then
	echo $usage 
	exit 1
fi

if [ "$1" == "android" ] ; then
	pushd jni
	#TODO 
	# - check ndk-build path
	# - check uapi
	~/Setups/android-ndk-r11c/ndk-build
	popd
	mkdir -p bin/android
	cp -R libs/armeabi-v7a bin/android/
elif [ "$1" == "linux" ] ; then
	cd src
	make
else
	echo "Invalid build option"
	echo $usage
	exit 1
fi
