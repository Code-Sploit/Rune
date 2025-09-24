#pragma once

#include <tables/constants.hpp>

namespace PrecomputedTables {

    // ----------------------------
    // Castle Data Structure
    // ----------------------------
    struct CastleData { 
        int rookFrom; 
        int rookTo; 
    };

    // ----------------------------
    // Precomputed Attack Tables
    // ----------------------------
    class AttackTable {
    public:
        // Piece attack tables
        Bitboard pieces[7][64];      // Precomputed attacks for all piece types
        Bitboard pawns[2][64];       // Precomputed pawn attacks (white/black)

        // Castling
        uint8_t castling[64][64];               // Castling rights per square pair
        CastleData castlingRookMoves[64][64];   // Corresponding rook moves

        // ----------------------------
        // Precomputation Functions
        // ----------------------------
        void preComputeKing();
        void preComputeKnight();
        void preComputeSliding(PieceType type);
        void preComputePawn(int color);
        void preComputeCastling();
        void preComputeAll(); // Run all precomputations at once

        // ----------------------------
        // Lookup Functions
        // ----------------------------
        Bitboard getPawnAttacks(int color, int square);
        Bitboard getKnightAttacks(int square);
        Bitboard getSlidingAttacks(int square, PieceType type);
        Bitboard getKingAttacks(int square);
    };

} // namespace PrecomputedTables