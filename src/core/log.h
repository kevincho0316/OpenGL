#pragma once
#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "GLTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) (fprintf(stdout, "[I] " __VA_ARGS__), fputc('\n', stdout))
#define LOGW(...) (fprintf(stderr, "[W] " __VA_ARGS__), fputc('\n', stderr))
#define LOGE(...) (fprintf(stderr, "[E] " __VA_ARGS__), fputc('\n', stderr))
#endif
