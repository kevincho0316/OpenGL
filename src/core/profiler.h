#pragma once
#include <chrono>

class profiler {
  private:
    std::chrono::steady_clock::time_point start_;
    std::chrono::duration<double> dt_;
    std::chrono::steady_clock::time_point lastFrame_;
    double maxFrame_ = 0.0;
    float shownMaxMs_ = 0.0f;

    int pastFrame_ = 0;
    double fps_ = -1.0;

  public:
    void initProfiler();
    void endFrame();
    double getFps() { return fps_; };
    float getMaxFps() { return shownMaxMs_; };
};
