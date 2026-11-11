#include "shaderLib.h"
#include "core/shader.h"
#include "platform/android/log.h"
#include "uniform.h"
#include <cstddef>

bool ShaderLib::add(const std::string &name, const std::string &vs,
                    const std::string &fs) {
    if (entries_.count(name))
        return false;

    entries_.emplace(name, ShaderEntry{vs, fs, Shader{}});
    return true;
}

ShaderEntry *ShaderLib::get(const std::string &name) {
    ShaderEntry *ptr = nullptr;
    auto i = entries_.find(name);
    if (i == entries_.end()) {
        return nullptr;
    }
    return &i->second;
}

// INIT_WINDOW, F5
void ShaderLib::recreateAll() {
    for (auto &[name, e] : entries_) {
        Shader s(e.vsPath, e.fsPath);
        if (!s.ok) {
            LOGE("ShaderLib::shader %s::has faied compiling", name.c_str());
            s.destroy();
            continue;
        } else {
            e.shader.destroy();
            e.shader = s;
        }
    }
}

// TERM_WINDOW
void ShaderLib::destroyAll() {
    for (auto &[name, e] : entries_) {
        e.shader.destroy();
    }
}
