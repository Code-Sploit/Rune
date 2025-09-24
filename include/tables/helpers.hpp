#pragma once

// ==================== Standard Library ====================
#include <iostream>

// ==================== Core & Constants ====================
#include <tables/constants.hpp>

namespace Helpers {

    // ----------------------------
    // Move / Bitboard Helpers
    // ----------------------------
    inline int getFrom(Move m)        { return m & FROM_MASK; }
    inline int getTo(Move m)          { return (m & TO_MASK) >> 6; }
    inline int getPromo(Move m)       { return (m & PROMO_MASK) >> 12; }
    inline int getColor(Piece p)      { return (p & PIECE_COLOR_MASK) >> 3; }
    inline int getType(Piece p)       { return (p & PIECE_TYPE_MASK); }

    inline bool isCapture(Move m)     { return (m & CAPTURE_MASK) != 0; }
    inline bool isPromo(Move m)       { return (m & PROMO_FLAG) != 0; }
    inline bool isEnpassant(Move m)   { return (m & ENPASSANT) != 0; }
    inline bool isCheck(Move m)       { return (m & CHECK) != 0; }
    inline bool isDoublePush(Move m)  { return (m & DOUBLE_PUSH) != 0; }
    inline bool isCastle(Move m)      { return (m & CASTLE) != 0; }

    inline Move createMove(int from, int to, int promo, int capture, int promoFlag,
                         int ep, int check, int doublePush, int castle)
    {
        return (((from) & 0x3F) |
                (((to) & 0x3F) << 6) |
                (((promo) & 0x7) << 12) |
                ((capture) ? CAPTURE_MASK : 0) |
                ((promoFlag) ? PROMO_FLAG : 0) |
                ((ep) ? ENPASSANT : 0) |
                ((check) ? CHECK : 0) |
                ((doublePush) ? DOUBLE_PUSH : 0) |
                ((castle) ? CASTLE : 0));
    }

    // ----------------------------
    // Piece Helpers
    // ----------------------------
    inline int makePiece(int type, int color) { return type | (color << 3); }
    bool isSlidingPiece(int type);

    // ----------------------------
    // Board Helpers
    // ----------------------------
    inline int rankOf(int square) { return square / 8; }
    inline int fileOf(int square) { return square % 8; }

    inline int popLsb(Bitboard &bb)
    {
        if (bb == 0) return -1; // or handle error
        int sq = __builtin_ctzll(bb);
        bb &= bb - 1; // clear the least significant bit
        return sq;
    }

    inline int clamp(int val, int minVal, int maxVal)
    {
        if (val < minVal) return minVal;
        if (val > maxVal) return maxVal;
        return val;
    }

    inline int findLargestOfThree(int a, int b, int c)
    {
        if (a >= b && a >= c) return 0;
        if (b >= a && b >= c) return 1;
        return 2; // otherwise c is largest
    }

    // ----------------------------
    // Utility Functions
    // ----------------------------
    inline void visualiseBitboard(Bitboard board)
    {
        std::cout << std::endl;

        for (int rank = 7; rank >= 0; rank--)
        {
            std::cout << " +---+---+---+---+---+---+---+---+" << std::endl << " ";

            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;
                std::cout << ((board & (1ULL << square)) ? "| X " : "|   ");
            }

            std::cout << "| " << rank + 1 << std::endl;
        }

        std::cout << " +---+---+---+---+---+---+---+---+" << std::endl;
        std::cout << "  a   b   c   d   e   f   g   h " << std::endl << std::endl;
    }

    // ----------------------------
    // Min / Max Templates
    // ----------------------------
    template<typename T>
    inline T min(T a, T b) { return (a < b) ? a : b; }

    template<typename T>
    inline T max(T a, T b) { return (a > b) ? a : b; }

} // namespace Helpers