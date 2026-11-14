#pragma once
#ifdef USE_GLES
#include <GLES3/gl3.h>
#else
// glad2 (glad/gl.h) or glad1 (glad/glad.h, xmake-repo's "glad" package)
#if __has_include(<glad/gl.h>)
#include <glad/gl.h>
#define GL_LOAD(getProc) gladLoadGL((GLADloadfunc)(getProc))
#else
#include <glad/glad.h>
#define GL_LOAD(getProc) gladLoadGLLoader((GLADloadproc)(getProc))
#endif
#endif
