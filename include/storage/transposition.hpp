#pragma once

// ==================== Core & Constants ====================
#include <tables/constants.hpp>

// --- Transposition table flags ---
#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA  2

namespace Transposition {

    // --------------------------------------------------------
    // TTEntry: Single entry in the transposition table
    // --------------------------------------------------------
    struct TTEntry {
        ZobristHash key;    // Position key
        int         depth;  // Depth at which entry was stored
        int         eval;   // Evaluation score
        int         flag;   // TT_EXACT / TT_ALPHA / TT_BETA
        Move        best_move; // Best move found
    };

    // --------------------------------------------------------
    // Table: Fixed-size transposition table
    // --------------------------------------------------------
    class Table {
    private:
        static constexpr size_t TT_SIZE = (1 << 8);
        TTEntry table[TT_SIZE]; // Array of TT entries

    public:
        // --- Operations ---
        bool probe(ZobristHash key, int depth, int alpha, int beta, int ply, int& out_score, Move& bestMove);
        void store(ZobristHash key, int depth, int eval, int flag, Move best_move, int ply);
        void clear();
    };

} // namespace Transposition