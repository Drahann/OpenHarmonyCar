#!/bin/bash

##########################################################################
# File Name    : compile.sh
# Encoding     : utf-8
# Author       : We-unite
# Email        : weunite1848@gmail.com
# Created Time : 2024-02-29 15:19:15
##########################################################################

set -e

if [ $UID -eq 0 ]; then
	echo "Please do not run this script as root"
	exit 1
fi

if [ $# -ne 1 ]; then
	echo "ERROR: $0 <arm64-v8a|x86_64>"
	exit 1
fi

arch=$1
link=static # or shared
native_path=/home/anxin/Workspace/newApp/native

if [ $arch == "arm64-v8a" ]; then
	export PATH=$native_path/build-tools/cmake/bin:$PATH
	cmake -B build -D OHOS_STL=c++_$link -D OHOS_ARCH=$arch -D OHOS_PLATFORM=OHOS -D CMAKE_TOOLCHAIN_FILE=$(find $native_path -name ohos.toolchain.cmake)
else
	cmake -B build -D OHOS_ARCH=$arch
fi

cmake --build build
