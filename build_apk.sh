#!/data/data/com.termux/files/usr/bin/bash
set -e
SDK=$ANDROID_SDK_ROOT
BT=$(ls -d $SDK/build-tools/* | sort -V | tail -1)
PLAT=$(ls -d $SDK/platforms/android-* | sort -V | tail -1)
JAR=$PLAT/android.jar
TARGET=${PLAT##*android-}; TARGET=${TARGET%%.*}
SO=$(find build -path "*android*" -name "*.so" | head -1)
KS=~/.android/debug.keystore
OUT=build/apk

[ -f "$JAR" ] || { echo "android.jar 없음: $JAR"; exit 1; }
[ -f "$SO" ]  || { echo ".so 없음 → xmake 먼저 실행"; exit 1; }
echo "build-tools: $BT"; echo "platform: $PLAT"; echo "so: $SO"

# 서명 키 (처음 한 번만 만들어짐)
if [ ! -f $KS ]; then
  mkdir -p ~/.android
  keytool -genkeypair -keystore $KS -alias androiddebugkey \
    -storepass android -keypass android -keyalg RSA -keysize 2048 \
    -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
fi

rm -rf $OUT; mkdir -p $OUT/lib/arm64-v8a
$BT/aapt2 link -o $OUT/unsigned.apk \
  -A assets \
  --manifest android/AndroidManifest.xml -I $JAR \
  --min-sdk-version 24 --target-sdk-version $TARGET
cp "$SO" $OUT/lib/arm64-v8a/libmain.so
(cd $OUT && zip -r unsigned.apk lib)
$BT/zipalign -f -p 4 $OUT/unsigned.apk $OUT/aligned.apk
$BT/apksigner sign --ks $KS --ks-pass pass:android \
  --out $OUT/gltest.apk $OUT/aligned.apk
echo "완료: $OUT/gltest.apk"
