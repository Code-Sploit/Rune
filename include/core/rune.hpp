#pragma once

// --- Standard Library ---
#include <memory>   // std::unique_ptr
#include <string>

// --- Tables & Utilities ---
#include <tables/constants.hpp>
#include <tables/helpers.hpp>
#include <utils/config.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>

// --- Core Modules ---
#include <core/board.hpp>
#include <core/movegen.hpp>
#include <core/attack.hpp>
#include <core/eval.hpp>
#include <core/search.hpp>

namespace Rune {
    struct AttackDiff {
        int color;
        int square;

        Bitboard oldAttacks;
    };
    // --- Snapshot of game state for history / undo ---
    class State {
    public:
        // Board & pieces
        Bitboard board[3][7];      // Pieces per type/side
        Bitboard occupancy[3];     // Side occupancy
        Piece boardGhost[64];      // Full piece array

        // Turn & move info
        int turn;
        int enpassantSquare;
        CastlingRights castlingRights;
        Piece capturedPiece;
        Move move;

        static constexpr int MAX_ATTACK_DIFFS = 64;

        int attackDiffCount = 0;

        // Incremental attack updator
        AttackDiff attackDiffs[MAX_ATTACK_DIFFS];

        Bitboard oldAttackMapFull[2];

        // Misc
        ZobristHash zobristKey;
        int fiftyMoveCounter;
    };

    // --- Main game class ---
    class Game {
    public:
        static constexpr size_t HISTORY_SIZE = 32768;

        // Board & pieces
        Bitboard board[3][7];       // Pieces by type/side
        Bitboard occupancy[3];      // Side occupancy
        Piece boardGhost[64];

        // Turn & move info
        int turn;
        int enpassantSquare;
        CastlingRights castlingRights;
        int ply;
        bool hasCastled[2];

        // Move generation
        Movegen::MoveList movelist;

        // Tables
        Transposition::Table transpositionTable;
        Repetition::Table repetitionTable;

        // Workers
        Attack::Worker attackWorker;
        Movegen::Worker movegenWorker;
        Evaluation::Worker evalWorker;
        Search::Worker searchWorker;

        // Configuration
        Config::Configuration config;

        // History
        int historyCount;
        bool isFirstLoad;
        std::unique_ptr<State[]> history;

        // Misc
        bool outOfOpeningBook;
        ZobristHash zobristKey;
        std::string pvLine;

        // Constructor / Destructor
        Game();
        ~Game();
    };

} // namespace Rune
