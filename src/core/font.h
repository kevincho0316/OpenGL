#pragma once
#include "stb_truetype.h" // IMPLEMENTATION 정의는 font.cpp에서만!
#include <vector>

class Font {
  private:
    static const int W = 512;
    static const int H = 512;
    std::vector<unsigned char> atlas_;
    stbtt_bakedchar cdata_[96];

  public:
    bool load(const char *path, float pixelHeight);

    const unsigned char *atlasData() const { return atlas_.data(); }
    int atlasWidth() const { return W; }
    int atlasHeight() const { return H; }

    bool getQuad(char c, float &x, float &y, stbtt_aligned_quad &q) const;
};
