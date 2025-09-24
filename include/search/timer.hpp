#pragma once

#include <chrono>

namespace Search::Timer {
    // --- Timer types ---
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    class Timer {
        private:
            TimePoint startTime { Clock::now() };
            TimePoint lastDepthStartedAt { Clock::now() };
            TimePoint lastDepthFinishedAt { Clock::now() };

            int thinkTime;

            bool timeUp = false;
        
        public:
            // --- Internal helpers ---
            void checkTimer();
            void setTimer(int time);

            double getTimer();
            double getElapsedTime();

            void startNewDepth();
            void finishDepth();

            bool isTimeUp();
    };
}