#include "util/Timer.hpp"

Timer::Timer()
    : lastFrame_(Clock::now())
{
}

void Timer::tick()
{
    const auto now = Clock::now();
    const std::chrono::duration<double> elapsed = now - lastFrame_;
    lastFrame_ = now;

    deltaSeconds_ = elapsed.count();
    totalSeconds_ += deltaSeconds_;
    fps_ = deltaSeconds_ > 0.0 ? 1.0 / deltaSeconds_ : 0.0;
}

double Timer::deltaSeconds() const
{
    return deltaSeconds_;
}

double Timer::totalSeconds() const
{
    return totalSeconds_;
}

double Timer::fps() const
{
    return fps_;
}
