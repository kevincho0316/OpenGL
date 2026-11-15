#pragma once
#include <chrono>

class profiler {
  private:
    std::chrono::steady_clock::time_point start;
    std::chrono::duration<double> dt_;
    std::chrono::steady_clock::time_point lastFrame_;
    double maxFrame_;
    float shownMaxMs_;

    int pastFrame_ = 0;
    double fps_ = -1.0;

  public:
    void initProfiler();
    void endFrame();
    double getFps() { return fps_; };
    float getMaxFps() { return shownMaxMs_; };
};
