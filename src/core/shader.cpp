#include "shader.h"
#include <cstdio>
#include <set>
#include <sstream>
#include <string>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/log.h>
#define SLOG(...) __android_log_print(ANDROID_LOG_ERROR, "gltest", __VA_ARGS__)
AAssetManager *Shader::assets = nullptr;
#else
#include <fstream>
#define SLOG(...) (fprintf(stderr, __VA_ARGS__), fputc('\n', stderr))
std::string Shader::assetDir = "assets/";
#endif

#ifdef USE_GLES
static const char *HEADER = "#version 300 es\nprecision mediump float;\n";
#else
static const char *HEADER = "#version 330 core\n";
#endif

// ---- 플랫폼별로 다른 부분은 이 함수 하나뿐 ----
std::string Shader::readFile(const std::string &path) {
#ifdef __ANDROID__
    if (!assets) {
        SLOG("readFile: AAssetManager가 설정되지 않음");
        return "";
    }
    AAsset *f = AAssetManager_open(assets, path.c_str(), AASSET_MODE_BUFFER);
    if (!f) {
        SLOG("readFile failed: %s", path.c_str());
        return "";
    }
    std::string s((const char *)AAsset_getBuffer(f), AAsset_getLength(f));
    AAsset_close(f);
    return s;
#else
    std::ifstream f(assetDir + path, std::ios::binary);
    if (!f) {
        SLOG("readFile failed: %s%s", assetDir.c_str(), path.c_str());
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
#endif
}

std::string Shader::preprocess(const std::string &path,
                               std::set<std::string> &seen) {
    if (!seen.insert(path).second)
        return ""; // 같은 파일 두 번 include 방지
    std::string dir = path.substr(0, path.find_last_of('/') + 1);
    std::istringstream in(readFile(path));
    std::string line, out;
    while (std::getline(in, line)) {
        if (line.rfind("#include", 0) == 0) {
            auto a = line.find('"'), b = line.rfind('"');
            out += preprocess(dir + line.substr(a + 1, b - a - 1), seen);
        } else {
            out += line + '\n';
        }
    }
    return out;
}

static unsigned int compile(GLenum type, const std::string &body) {
    const char *p = body.c_str();
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        SLOG("%s shader error: %s",
             type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
    }
    return s;
}

Shader::Shader(const std::string &vsDir, const std::string &fsDir) {
    // 단계마다 seen 따로 (VS/FS가 같은 파일을 include 해도 둘 다 들어가도록)
    std::set<std::string> vsSeen, fsSeen;
    std::string VSsrc = HEADER + preprocess(vsDir, vsSeen);
    std::string FSsrc = HEADER + preprocess(fsDir, fsSeen);
    unsigned int vs = compile(GL_VERTEX_SHADER, VSsrc);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, FSsrc);
    ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);
    int ok;
    glGetProgramiv(ID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(ID, 512, nullptr, log);
        SLOG("link error: %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Shader::use() const { glUseProgram(ID); }

void Shader::destroy() {
    if (ID)
        glDeleteProgram(ID);
    ID = 0;
}

void Shader::setBool(const std::string &n, bool v) const {
    glUniform1i(glGetUniformLocation(ID, n.c_str()), (int)v);
}
void Shader::setInt(const std::string &n, int v) const {
    glUniform1i(glGetUniformLocation(ID, n.c_str()), v);
}
void Shader::setFloat(const std::string &n, float v) const {
    glUniform1f(glGetUniformLocation(ID, n.c_str()), v);
}
void Shader::setVec2(const std::string &n, glm::vec2 v) const {
    glUniform2f(glGetUniformLocation(ID, n.c_str()), v.x, v.y);
}
void Shader::setVec3(const std::string &n, glm::vec3 v) const {
    glUniform3f(glGetUniformLocation(ID, n.c_str()), v.x, v.y, v.z);
}
void Shader::setVec4(const std::string &n, glm::vec4 v) const {
    glUniform4f(glGetUniformLocation(ID, n.c_str()), v.x, v.y, v.z, v.w);
}
void Shader::setMat4(const std::string &n, const glm::mat4 &m) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE,
                       &m[0][0]);
}
GLuint Shader::getUniLoc(const std::string &n) const {
    return glGetUniformLocation(ID, n.c_str());
}
