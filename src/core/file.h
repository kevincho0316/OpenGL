#pragma once
#include <vector>
#ifdef __ANDROID__
#include <android/asset_manager.h>
void setAssetManager(AAssetManager *mgr); // android_main에서 한 번 호출
#endif

// path는 assets 폴더 기준 (예: "fonts/x.ttf"). 실패하면 빈 벡터
std::vector<unsigned char> readFile(const char *path);
