#include "core/render.h"
#include "core/shader.h"
#include "core/shaderLib.h"
#include <algorithm>
#include <cstddef>

void Renderer::setPassState(Pass p) {
    switch (p) {
    case Pass::Opaque:
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        break;
    case Pass::Transparent:
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE); // 깊이는 검사만, 쓰지는 않음
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case Pass::Emissive:
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied
        break;
    case Pass::Overlay:
    case Pass::UI:
        glDisable(GL_DEPTH_TEST); // 항상 맨 위에
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

void Renderer::flush(Pass last) {
    // 패스 순으로 정렬. stable이라 같은 패스 안에서는 submit 순서 유지
    std::stable_sort(
        queue_.begin(), queue_.end(),
        [](const DrawCmd &a, const DrawCmd &b) { return a.pass < b.pass; });

    bool first = true;
    Pass curPass{};
    const ShaderEntry *bound = nullptr;

    size_t drawn = 0;
    for (const DrawCmd &c : queue_) {
        if (c.pass > last)
            break; // sorted: the rest belongs to later passes
        drawn++;
        if (!c.shaderEntry || !c.shaderEntry->shader.ID)
            continue; // 컨텍스트 손실 직후 등

        const Shader &s = c.shaderEntry->shader;

        if (first || c.pass != curPass) {
            setPassState(c.pass);
            curPass = c.pass;
            first = false;
        }
        if (c.shaderEntry != bound) {
            c.shaderEntry->shader.use();
            for (const auto &u : frameUniforms)
                apply(s, u);
            bound = c.shaderEntry;
            for (const auto &[name, v] : c.shaderEntry->params)
                apply(s, name, v);
        }
        for (const auto &u : c.uniforms)
            apply(s, u);

        glBindVertexArray(c.vao);
        if (c.instances > 0)
            glDrawArraysInstanced(c.mode, c.first, c.count, c.instances);
        else
            glDrawArrays(c.mode, c.first, c.count);
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);     // 다음 프레임 glClear가 깊이를 지울 수 있도록
    glDisable(GL_DEPTH_TEST); // 추가
    glDisable(GL_BLEND);
    queue_.erase(queue_.begin(), queue_.begin() + (std::ptrdiff_t)drawn);
}
