#pragma once

#include <tables/constants.hpp>

#include <chrono>

namespace Search::Types {
    // --------------------------------------------------------
    // MoveScore: Associates a move with a score
    // --------------------------------------------------------
    struct MoveScore {
        Move move;
        int  score;
    };

    // --------------------------------------------------------
    // Move request type for search
    // --------------------------------------------------------
    enum MoveRequestType {
        QUIESCENSE,
        NEGAMAX
    };
}