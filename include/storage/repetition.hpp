#pragma once

// ==================== Core & Constants ====================
#include <tables/constants.hpp>

namespace Rune {
    class Game; // Forward declaration
}

namespace Repetition {

    // --------------------------------------------------------
    // Table: Tracks position repetitions for draw detection
    // --------------------------------------------------------
    class Table {
    private:
        static constexpr size_t REPETITION_SIZE = 16384;

        ZobristHash stack[REPETITION_SIZE];  // History of position hashes
        size_t      start = 0;                // Index of first valid hash
        size_t      count = 0;                // Number of stored hashes

    public:
        int fiftyMoveCounter = 0;

        // --- Stack operations ---
        void push(ZobristHash hash);    // Push a new position hash
        void pop();                      // Pop the last position hash
        void clear();                    // Clear the repetition table

        // --- Repetition checks ---
        bool checkThreefold(ZobristHash hash);       // Has this hash occurred three times?
        bool checkThreefoldRecent(size_t recentMoves);
        bool checkFiftyMoveRule();                   // Fifty-move rule detection
    };

} // namespace Repetition
