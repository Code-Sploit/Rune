#pragma once

#include <cstdint>
#include <tables/constants.hpp>
#include <tables/magic_bitboards.hpp>

namespace Magic {

    // ----------------------------
    // Bishop Attacks
    // ----------------------------
    inline Bitboard getBishopAttacks(int square, Bitboard occupancy) {
        Bitboard blockers = occupancy & bishopMasks[square];
        std::size_t index = static_cast<std::size_t>((blockers * bishopMagics[square]) >> bishopShifts[square]);
        return bishopAttackTables[square][index];
    }

    // ----------------------------
    // Rook Attacks
    // ----------------------------
    inline Bitboard getRookAttacks(int square, Bitboard occupancy) {
        Bitboard blockers = occupancy & rookMasks[square];
        std::size_t index = static_cast<std::size_t>((blockers * rookMagics[square]) >> rookShifts[square]);
        return rookAttackTables[square][index];
    }

    // ----------------------------
    // Queen Attacks = Bishop + Rook
    // ----------------------------
    inline Bitboard getQueenAttacks(int square, Bitboard occupancy) {
        return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
    }

} // namespace Magic