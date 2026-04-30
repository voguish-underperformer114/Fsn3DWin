#pragma once

#include <chrono>

class Timer {
public:
    Timer();

    void tick();
    double deltaSeconds() const;
    double totalSeconds() const;
    double fps() const;

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point lastFrame_;
    double deltaSeconds_ = 0.0;
    double totalSeconds_ = 0.0;
    double fps_ = 0.0;
};
