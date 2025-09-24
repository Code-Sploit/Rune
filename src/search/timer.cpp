#include <chrono>

#include <search/timer.hpp>
#include <search/config.hpp>

namespace Search::Timer {
    void Timer::checkTimer()
    {
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds> (
            Clock::now() - startTime
        ).count();

        if (elapsedMs >= (thinkTime - Config::searchThinkTimeMargin))
            timeUp = true;
    }

    void Timer::setTimer(int time)
    {
        this->thinkTime = time;
        this->startTime = Clock::now();
    }

    double Timer::getTimer()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds> (
            lastDepthFinishedAt - lastDepthStartedAt
        ).count();
    }

    double Timer::getElapsedTime()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds> (
            Clock::now() - startTime
        ).count();
    }

    void Timer::startNewDepth()
    {
        lastDepthStartedAt = Clock::now();
    }

    void Timer::finishDepth()
    {
        lastDepthFinishedAt = Clock::now();
    }

    bool Timer::isTimeUp()
    {
        return this->timeUp;
    }
}