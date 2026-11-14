#pragma once
// HDR render target + post-processing chain:
//   scene (RGBA16F) -> bloom (bright pass, 13-tap down, 3x3 tent up)
//   -> colour boost -> tonemap -> sRGB -> the framebuffer that was bound
//   when begin() was called (default FB on Android, blit FBO on desktop).
#include "core/shaderLib.h"
#include "gl.h"
#include <vector>

class PostFx {
  public:
    bool enabled = true; // false = draw straight to the bound framebuffer

    // Blender Glare (Bloom): Threshold 1.0, Smoothness 0.1, Strength 1.0,
    // Size 0.5
    float bloomThreshold = 1.0f;
    float bloomKnee = 0.1f;
    float bloomStrength = 1.0f;
    // 0..1. Mapped to mip count (round(2 + 8 * size), 0.5 -> 6) and tent
    // radius (0.5 + size texels, 0.5 -> 1.0)
    float bloomSize = 0.5f;
    float saturation = 1.3f; // "Color Boost", luminance preserved
    int tonemap = 1;         // 0 = clamp only, 1 = ACES

    bool shaderRegister(ShaderLib &lib);
    bool init(ShaderLib &lib);
    void destroy();

    // Bind the HDR target (remembers the currently bound framebuffer)
    void begin(int w, int h);
    // Bloom + composite into the remembered framebuffer, which stays bound
    void end();

  private:
    struct Mip {
        GLuint fbo = 0, tex = 0;
        int w = 0, h = 0;
    };
    void ensureTargets(int w, int h);
    void destroyTargets();

    const ShaderEntry *down_ = nullptr, *up_ = nullptr, *composite_ = nullptr;
    GLuint vao_ = 0; // empty VAO for the fullscreen triangle
    GLuint fbo_ = 0, color_ = 0, depth_ = 0;
    std::vector<Mip> mips_;
    int w_ = 0, h_ = 0;
    GLint target_ = 0;    // framebuffer bound at begin()
    bool active_ = false; // begin() bound the HDR target this frame
    bool hdr_ = true;     // false = RGBA16F not renderable, RGBA8 fallback
};
