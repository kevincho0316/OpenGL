#define STB_TRUETYPE_IMPLEMENTATION // 프로젝트 전체에서 이 파일 딱 한 곳
#include "font.h"
#include "log.h"
#include "file.h"

bool Font::load(const char *path, float pixelHeight) {
    std::vector<unsigned char> ttf = readFile(path); // 반환형에 맞게 조정
    if (ttf.empty()) {
        LOGE("FONT::FAIL:TO:READ: %s", path);
        return false;
    }

    atlas_.assign(W * H, 0);
    int r = stbtt_BakeFontBitmap(ttf.data(), 0, pixelHeight, atlas_.data(), W,
                                 H, 32, 96, cdata_);
    if (r <= 0) {
        LOGE("FONT::AtlasIsToSmall(Result %d)", r);
        return false;
    }
    pixelHeight_ = pixelHeight;
    return true;
}

bool Font::getQuad(char c, float &x, float &y, stbtt_aligned_quad &q) const {
    if (c < 32 || c >= 128)
        return false;
    stbtt_GetBakedQuad(cdata_, W, H, c - 32, &x, &y, &q, 1);
    return true;
}

float Font::textWidth(const char *text) const {
    float x = 0.0f, y = 0.0f;
    stbtt_aligned_quad q;
    for (const char *p = text; *p; ++p)
        getQuad(*p, x, y, q); // x 가 글자 advance 만큼 전진
    return x;
}
