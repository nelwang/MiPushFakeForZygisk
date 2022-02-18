#!/usr/bin/env bash


build_mode="${1:-release}"

cd "$(dirname "$0")"

pushd zygisk
rm -fr libs
debug_mode=1
if [[ "$build_mode" == "release" ]]; then
    debug_mode=0
fi
/opt/android_sdk/ndk/21.4.7075529/ndk-build -j48 NDK_DEBUG=$debug_mode
popd

mkdir -p magisk/zygisk
for arch in arm64-v8a armeabi-v7a x86 x86_64
do
    cp "zygisk/libs/$arch/libmipushfake.so" "magisk/zygisk/$arch.so"
done

pushd magisk
version="$(grep '^version=' module.prop  | cut -d= -f2)"
rm -f "../mipushfake-zygisk-$version.zip" 
zip -r9 "../mipushfake-zygisk-$version.zip" .
popd
