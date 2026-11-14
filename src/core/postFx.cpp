#include "core/postFx.h"
#include "core/log.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

static const int kMaxMips = 8;

bool PostFx::shaderRegister(ShaderLib &lib) {
    bool a = lib.add("BloomDown", "shaders/Post.vert", "shaders/BloomDown.frag");
    bool b = lib.add("BloomUp", "shaders/Post.vert", "shaders/BloomUp.frag");
    bool c = lib.add("PostComposite", "shaders/Post.vert",
                     "shaders/PostComposite.frag");
    return a && b && c;
}

bool PostFx::init(ShaderLib &lib) {
    down_ = lib.get("BloomDown");
    up_ = lib.get("BloomUp");
    composite_ = lib.get("PostComposite");
    if (!down_ || !up_ || !composite_) {
        LOGE("PostFx::NEED to register the shaders to shader lib");
        return false;
    }
    glGenVertexArrays(1, &vao_);
    return true;
}

void PostFx::destroyTargets() {
    for (Mip &m : mips_) {
        if (m.fbo)
            glDeleteFramebuffers(1, &m.fbo);
        if (m.tex)
            glDeleteTextures(1, &m.tex);
    }
    mips_.clear();
    if (fbo_)
        glDeleteFramebuffers(1, &fbo_);
    if (color_)
        glDeleteTextures(1, &color_);
    if (depth_)
        glDeleteRenderbuffers(1, &depth_);
    fbo_ = color_ = depth_ = 0;
    w_ = h_ = 0;
}

void PostFx::destroy() {
    destroyTargets();
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
    active_ = false;
}

static GLuint makeColorTex(int w, int h, bool hdr) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    if (hdr)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA,
                     GL_HALF_FLOAT, nullptr);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

// Leaves the last created framebuffer bound; caller rebinds
void PostFx::ensureTargets(int w, int h) {
    if (fbo_ && w == w_ && h == h_)
        return;
    destroyTargets();
    w_ = w;
    h_ = h;

    glGenRenderbuffers(1, &depth_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_);
    // RGBA16F is always renderable on GL 3.3; on GLES 3.0 it needs
    // EXT_color_buffer_half_float / EXT_color_buffer_float -> fall back to LDR
    for (int attempt = 0; attempt < 2; attempt++) {
        color_ = makeColorTex(w, h, hdr_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color_, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
            break;
        glDeleteTextures(1, &color_);
        color_ = 0;
        if (!hdr_) {
            LOGE("PostFx: scene framebuffer incomplete");
            break;
        }
        LOGW("PostFx: RGBA16F not renderable, falling back to RGBA8 (no HDR)");
        hdr_ = false;
    }

    int mw = w, mh = h;
    for (int i = 0; i < kMaxMips; i++) {
        mw = std::max(mw / 2, 1);
        mh = std::max(mh / 2, 1);
        Mip m;
        m.w = mw;
        m.h = mh;
        m.tex = makeColorTex(mw, mh, hdr_);
        glGenFramebuffers(1, &m.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m.tex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            LOGE("PostFx: bloom mip %d framebuffer incomplete", i);
        mips_.push_back(m);
        if (mw == 1 && mh == 1)
            break;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostFx::begin(int w, int h) {
    active_ = false;
    if (!enabled || !vao_ || w <= 0 || h <= 0)
        return;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &target_);
    ensureTargets(w, h);
    if (!color_) { // could not build a scene target: render directly
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)target_);
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    active_ = true;
}

void PostFx::end() {
    if (!active_)
        return;
    active_ = false;

    bool shadersOk = down_->shader.ID && up_->shader.ID && composite_->shader.ID;
    if (!shadersOk || mips_.empty()) { // e.g. shader compile error: plain copy
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)target_);
        glBlitFramebuffer(0, 0, w_, h_, 0, 0, w_, h_, GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)target_);
        return;
    }

    float size = glm::clamp(bloomSize, 0.0f, 1.0f);
    int n = glm::clamp((int)std::lround(2.0f + 8.0f * size), 1,
                       (int)mips_.size());
    float radius = 0.5f + size;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindVertexArray(vao_);
    glActiveTexture(GL_TEXTURE0);

    // ---- bright pass + downsample chain ----
    const Shader &down = down_->shader;
    down.use();
    down.setInt("uSrc", 0);
    down.setFloat("uThreshold", bloomThreshold);
    down.setFloat("uKnee", bloomKnee);
    GLuint srcTex = color_;
    int sw = w_, sh = h_;
    for (int i = 0; i < n; i++) {
        const Mip &m = mips_[i];
        glBindFramebuffer(GL_FRAMEBUFFER, m.fbo);
        glViewport(0, 0, m.w, m.h);
        down.setVec2("uTexel", glm::vec2(1.0f / sw, 1.0f / sh));
        down.setInt("uPrefilter", i == 0 ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D, srcTex);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        srcTex = m.tex;
        sw = m.w;
        sh = m.h;
    }

    // ---- upsample chain: mip i added onto mip i-1 ----
    const Shader &up = up_->shader;
    up.use();
    up.setInt("uSrc", 0);
    up.setFloat("uRadius", radius);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (int i = n - 1; i > 0; i--) {
        const Mip &src = mips_[i], &dst = mips_[i - 1];
        glBindFramebuffer(GL_FRAMEBUFFER, dst.fbo);
        glViewport(0, 0, dst.w, dst.h);
        up.setVec2("uTexel", glm::vec2(1.0f / src.w, 1.0f / src.h));
        glBindTexture(GL_TEXTURE_2D, src.tex);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glDisable(GL_BLEND);

    // ---- composite: scene + bloom -> colour boost -> tonemap -> sRGB ----
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)target_);
    glViewport(0, 0, w_, h_);
    const Shader &comp = composite_->shader;
    comp.use();
    comp.setInt("uScene", 0);
    comp.setInt("uBloom", 1);
    comp.setVec2("uBloomTexel",
                 glm::vec2(1.0f / mips_[0].w, 1.0f / mips_[0].h));
    comp.setFloat("uBloomRadius", radius);
    // The additive chain sums n mips: divide so strength 1 conserves energy
    comp.setFloat("uBloomStrength", bloomStrength / (float)n);
    comp.setFloat("uSaturation", saturation);
    comp.setInt("uTonemap", tonemap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mips_[0].tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}
