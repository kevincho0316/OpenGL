adb logcat -c                          # 이전 로그 지우기
am start -n com.kevin.gltest/android.app.NativeActivity
adb logcat -s GLTest                   # 내 태그만
adb logcat -d > log.txt 
echo "{LOG::FILE GENERATED}"
#adb logcat --pid=$(adb shell pidof com.kevin.gltest)   # 내 앱 프로세스 전부
#adb logcat '*:E'                                       # 에러 이상만
#adb logcat -b crash                                    # 크래시만
#adb logcat -d > log.txt                                # 파일로 저장 후 vim으로 보기
