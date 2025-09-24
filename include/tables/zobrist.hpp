#pragma once

#include <string>
#include <core/rune.hpp>  // Assumes Game, Move, ZobristHash, etc. are defined here
#include <core/board.hpp>

namespace Rune {
    class Game; // Forward declaration
}

namespace Zobrist {

    // ----------------------------
    // Constants
    // ----------------------------
    constexpr int numPieceTypes = 12;
    constexpr int numSquares    = 64;
    constexpr int numCastling   = 16;
    constexpr int numEnPassant  = 8;

    // ----------------------------
    // Zobrist Hash Functions
    // ----------------------------

    // Compute the Zobrist hash of the current position
    ZobristHash compute(Rune::Game& game);

    // Update hash after a board change
    void updateBoard(Rune::Game& game);

    // Update hash after a move, given the previous state
    void updateMove(Rune::Game& game, Move move, Rune::State& oldState);

    // Initialize Zobrist random keys
    void init();

    // Convert a hash to a string representation (for debugging)
    std::string hashToString(ZobristHash hash);

} // namespace Zobrist