#pragma once

// ==================== Standard Library ====================
#include <ctime>
#include <array>
#include <cmath>
#include <chrono>
#include <vector>

// ==================== Core & Constants ====================
#include <core/movegen.hpp>
#include <tables/constants.hpp>

#include <search/timer.hpp>
#include <search/types.hpp>
#include <search/tables.hpp>
#include <search/ordering.hpp>
#include <search/config.hpp>

namespace Rune {
    class Game; // Forward declaration
}

namespace Search {
    // --------------------------------------------------------
    // Worker: Handles search algorithms and move ordering
    // --------------------------------------------------------
    class Worker {
    private:
        Timer::Timer timerManager;
        MoveOrdering::Manager orderManager;

        bool isNullMovePruneSafe(Rune::Game& game, Movegen::MoveList& movelist);

    public:
        // --- Search algorithms ---
        int quiescense(Rune::Game& game, int depth, int alpha, int beta, int ply, std::vector<Move>& pv);
        int negamax(Rune::Game& game, int depth, int alpha, int beta, int ply, std::vector<Move>& pv);

        // --- Entry point ---
        Move searchPosition(Rune::Game& game, int initialDepth, int thinkTimeMs);
    };

} // namespace Search