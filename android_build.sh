cd ~/projects/OpenGL && chmod +x push.sh
xmake f -p android -a arm64-v8a --ndk=$ANDROID_NDK --android_sdk=$ANDROID_SDK_ROOT --ndk_sdkver=24 -c -y
xmake && ./build_apk.sh
cp build/apk/gltest.apk ~/storage/downloads/
termux-open --chooser ~/storage/downloads/gltest.apk
