#pragma once

// ==================== Standard Library ====================
#include <memory>    // std::unique_ptr
#include <string>

// ==================== Tables & Utilities ====================
#include <tables/constants.hpp>
#include <tables/helpers.hpp>
#include <utils/config.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>

// ==================== Core Modules ====================
#include <core/board.hpp>
#include <core/movegen.hpp>
#include <core/attack.hpp>
#include <core/eval.hpp>
#include <search/search.hpp>

namespace Rune {
    static constexpr Bitboard FILE_MASKS[8] = {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0404040404040404ULL,
        0x0808080808080808ULL,
        0x1010101010101010ULL,
        0x2020202020202020ULL,
        0x4040404040404040ULL,
        0x8080808080808080ULL
    };

    // --------------------------------------------------------
    // AttackDiff: Represents a delta in attack maps
    // --------------------------------------------------------
    struct AttackDiff {
        int color;
        int square;
        Bitboard oldAttacks;
    };

    // --------------------------------------------------------
    // State: Snapshot of game state (for history / undo)
    // --------------------------------------------------------
    class State {
        public:
            // --- Board & pieces ---
            Bitboard board[3][7];        // Pieces per type/side
            Bitboard occupancy[3];       // Side occupancy
            Piece    boardGhost[64];     // Full piece array

            // --- Turn & move info ---
            int            turn;
            int            enpassantSquare;
            int            fiftyMoveCounter;
            int            attackDiffCount = 0;
            
            CastlingRights castlingRights;
            Piece          capturedPiece;
            Move           move;

            // --- Incremental attack tracking ---
            static constexpr int MAX_ATTACK_DIFFS = 64;

            AttackDiff attackDiffs[MAX_ATTACK_DIFFS];
            Bitboard   oldAttackMapFull[2];

            // --- Misc ---
            ZobristHash zobristKey;
    };

    // --------------------------------------------------------
    // Game: Main game manager
    // --------------------------------------------------------
    class Game {
        public:
            static constexpr size_t HISTORY_SIZE = 32768;

            // --- Board & pieces ---
            Bitboard board[3][7];        // Pieces by type/side
            Bitboard occupancy[3];       // Side occupancy
            Piece    boardGhost[64];

            // --- Turn & move info ---
            int            turn;
            int            enpassantSquare;
            CastlingRights castlingRights;
            int            ply;
            bool           hasCastled[2];

            // --- Move generation ---
            Movegen::MoveList movelist;

            // --- Tables ---
            Transposition::Table transpositionTable;
            Repetition::Table    repetitionTable;

            // --- Workers ---
            Attack::Worker     attackWorker;
            Movegen::Worker    movegenWorker;
            Evaluation::Worker evalWorker;
            Search::Worker     searchWorker;

            // --- Configuration ---
            Config::Configuration config;

            // --- History ---
            int                          historyCount;
            bool                         isFirstLoad;
            std::unique_ptr<State[]>     history;

            // --- Misc ---
            bool         outOfOpeningBook;
            ZobristHash  zobristKey;
            std::string  pvLine;

            // --- Constructor / Destructor ---
            Game();
            ~Game();
    };

} // namespace Rune