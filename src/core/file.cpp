#include "file.h"
#include "log.h"
#include <cstdio>
#include <string>
#ifdef __ANDROID__

static AAssetManager *g_assets = nullptr;
void setAssetManager(AAssetManager *mgr) { g_assets = mgr; }

std::vector<unsigned char> readFile(const char *path) {
    std::vector<unsigned char> data;
    while (*path == '/')
        path++; // "/fonts/x.ttf" 도 허용 (AssetManager 는 앞 '/' 를 못 찾음)
    if (!g_assets) {
        LOGE("readFile: AssetManager 미설정 (setAssetManager 호출 전?)");
        return data;
    }
    AAsset *a = AAssetManager_open(g_assets, path, AASSET_MODE_BUFFER);
    if (!a) {
        LOGE("readFile: asset 열기 실패: '%s'", path);
        return data;
    }
    const unsigned char *p = (const unsigned char *)AAsset_getBuffer(a);
    data.assign(p, p + AAsset_getLength(a));
    AAsset_close(a);
    return data;
}

#else

std::vector<unsigned char> readFile(const char *path) {
    std::vector<unsigned char> data;
    while (*path == '/')
        path++; // 안드로이드와 같은 규칙
    std::string full = std::string("assets/") + path;
    FILE *f = fopen(full.c_str(), "rb");
    if (!f)
        return data;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    data.resize(size);
    if (fread(data.data(), 1, size, f) != (size_t)size)
        data.clear();
    fclose(f);
    return data;
}

#endif
