#pragma once

// ==================== Standard Library ====================
#include <string>
#include <algorithm> // std::fill

// ==================== Core & Constants ====================
#include <core/attack.hpp>
#include <tables/constants.hpp>

namespace Rune {
    class Game; // Forward declaration
}

namespace Movegen {

    // --------------------------------------------------------
    // MoveList: Container for moves
    // --------------------------------------------------------
    class MoveList {
    private:
        int   count;
        Move  moves[MAX_MOVES];

    public:
        // --- Constructor ---
        MoveList() : count(0) {
            std::fill(std::begin(moves), std::end(moves), 0);
        }

        // --- Accessors ---
        int size() const { return count; }
        void setsize(int size) { this->count = size; }

        Move operator[](std::size_t index) const { return moves[index]; }
        Move& operator[](std::size_t index) { return moves[index]; }

        // --- Modifiers ---
        void add(Move move) {
            if (count < MAX_MOVES) {
                moves[count++] = move;
            }
        }

        void clear() { count = 0; }
    };

    // --------------------------------------------------------
    // Worker: Generates moves for all pieces and special moves
    // --------------------------------------------------------
    class Worker {
    public:
        // --- Internal helpers ---
        void addPromotionMoves(MoveList& moves, int from, int to, int isCapture);
        bool canCastleThroughBitboard(int square, Bitboard occupancy, Bitboard enemyAttacks);

        // --- Piece move generators ---
        void getPawnMoves(Rune::Game& game, MoveList& moves, bool onlyCaptures);
        void getKnightMoves(Rune::Game& game, MoveList& moves, bool onlyCaptures);
        void getKingMoves(Rune::Game& game, MoveList& moves, bool onlyCaptures);
        void getSlidingMoves(Rune::Game& game, MoveList& moves, PieceType type, bool onlyCaptures);

        // --- Castling ---
        bool canCastleThrough(Rune::Game& game, int square, Bitboard occupancy, Bitboard enemyAttacks);
        void getCastleMoves(Rune::Game& game, MoveList& moves);

        // --- Full move generation ---
        void getPseudoMoves(Rune::Game& game, MoveList& moves, bool onlyCaptures);
        void getLegalMoves(Rune::Game& game, MoveList& moves, bool onlyCaptures);
    };

} // namespace Movegen