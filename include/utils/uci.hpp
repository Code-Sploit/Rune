#pragma once

#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include <core/board.hpp>

namespace Rune {
    class Game; // Forward declaration
}

namespace UCI {

    // ----------------------------
    // Engine Info
    // ----------------------------
    constexpr const char* uciVersion = "Rune V1.0.0";
    constexpr const char* uciAuthor  = "Samuel 't Hart";

    // ----------------------------
    // Debug & Output
    // ----------------------------
    void debug(const char* file, const char* format, ...);
    void printSearchResult(int depth, int score, int timeMs, bool isMate, std::string pvCurrent);

    // ----------------------------
    // UCI Main Loop
    // ----------------------------
    void uciLoop(Rune::Game& game);

} // namespace UCI