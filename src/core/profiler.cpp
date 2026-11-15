#include "profiler.h"
#include <chrono>
#include <thread>
void profiler::initProfiler() {
    start_ = lastFrame_ = std::chrono::steady_clock::now();
}

// can be optimized
//
void profiler::endFrame() {
    std::chrono::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> samplingT = end - start_;

    pastFrame_++;
    dt_ = end - lastFrame_;
    lastFrame_ = end;
    if (dt_.count() > maxFrame_) {
        maxFrame_ = dt_.count();
    }

    if (samplingT.count() > 0.5) {
        fps_ = static_cast<float>(pastFrame_) / samplingT.count();
        start_ = end;
        pastFrame_ = 0;

        shownMaxMs_ = static_cast<float>(maxFrame_ * 1000.0);
        maxFrame_ = 0.0;
    }
}
