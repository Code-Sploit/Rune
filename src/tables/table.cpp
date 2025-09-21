#include <tables/table.hpp>
#include <tables/magic.hpp>

#include <cmath>    // for std::abs
#include <cstdint>

namespace PrecomputedTables {

    constexpr int KNIGHT_OFFSETS[8] = {17, 10, 6, 15, -6, -15, -17, -10};

    // ----------------------------
    // King attacks
    // ----------------------------
    void AttackTable::preComputeKing()
    {
        for (int square = 0; square < 64; ++square) {
            Bitboard moves = 0ULL;
            int rank = square / 8;
            int file = square % 8;

            int offsets[8] = {8, 9, 1, -7, -8, -9, -1, 7};

            for (int i = 0; i < 8; ++i) {
                int target = square + offsets[i];
                if (target < 0 || target >= 64) continue;

                int target_rank = target / 8;
                int target_file = target % 8;

                if (std::abs(target_rank - rank) <= 1 && std::abs(target_file - file) <= 1) {
                    moves |= 1ULL << target;
                }
            }

            this->pieces[KING][square] = moves;
        }
    }

    // ----------------------------
    // Knight attacks
    // ----------------------------
    void AttackTable::preComputeKnight()
    {
        for (int square = 0; square < 64; ++square)
        {
            int rank = square / 8;
            int file = square % 8;
            Bitboard moves = 0ULL;

            // All 8 knight offsets
            constexpr int offsets[8] = {17, 15, 10, 6, -17, -15, -10, -6};

            for (int i = 0; i < 8; ++i)
            {
                int target = square + offsets[i];

                if (target < 0 || target >= 64) continue;

                int targetRank = target / 8;
                int targetFile = target % 8;

                // Ensure the move does not wrap around the board horizontally
                if (std::abs(targetFile - file) > 2) continue;
                if (std::abs(targetRank - rank) > 2) continue;

                moves |= 1ULL << target;
            }

            this->pieces[KNIGHT][square] = moves;
        }
    }

    // ----------------------------
    // Sliding attacks (bishop, rook, queen)
    // ----------------------------
    void AttackTable::preComputeSliding(int piece_type) {
        for (int square = 0; square < 64; ++square) {
            Bitboard attacks = 0ULL;
            switch (piece_type) {
                case BISHOP:
                    attacks = Magic::getBishopAttacks(square, 0ULL);
                    break;
                case ROOK:
                    attacks = Magic::getRookAttacks(square, 0ULL);
                    break;
                case QUEEN:
                    attacks = Magic::getQueenAttacks(square, 0ULL);
                    break;
                default:
                    continue;
            }

            this->pieces[piece_type][square] = attacks;
        }
    }

    // ----------------------------
    // Pawn attacks
    // ----------------------------
    void AttackTable::preComputePawn(int color) {
        int left_offset  = (color == WHITE) ? 7  : -9;
        int right_offset = (color == WHITE) ? 9  : -7;

        for (int square = 0; square < 64; ++square) {
            int file = square % 8;
            Bitboard attacks = 0ULL;

            if (file > 0) {
                int target = square + left_offset;
                if (target >= 0 && target < 64) attacks |= 1ULL << target;
            }
            if (file < 7) {
                int target = square + right_offset;
                if (target >= 0 && target < 64) attacks |= 1ULL << target;
            }

            this->pawns[color][square] = attacks;
        }
    }

    // ----------------------------
    // Castling rights lookup
    // ----------------------------
    void AttackTable::preComputeCastling()
    {
        for (int from = 0; from < 64; ++from) {
            for (int to = 0; to < 64; ++to) {
                uint8_t rights = CASTLING_ALL;
                CastleData rookMove = { -1, -1 }; // default: invalid

                if (from == 4) rights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
                if (from == 60) rights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);

                if (from == 0) rights &= ~WHITE_QUEENSIDE;
                if (from == 7) rights &= ~WHITE_KINGSIDE;
                if (from == 56) rights &= ~BLACK_QUEENSIDE;
                if (from == 63) rights &= ~BLACK_KINGSIDE;

                if (to == 0) rights &= ~WHITE_QUEENSIDE;
                if (to == 7) rights &= ~WHITE_KINGSIDE;
                if (to == 56) rights &= ~BLACK_QUEENSIDE;
                if (to == 63) rights &= ~BLACK_KINGSIDE;

                // Precompute rook moves for castling
                if (from == 4 && to == 6)       rookMove = { 7, 5 };   // White kingside
                else if (from == 4 && to == 2)  rookMove = { 0, 3 };   // White queenside
                else if (from == 60 && to == 62) rookMove = { 63, 61 }; // Black kingside
                else if (from == 60 && to == 58) rookMove = { 56, 59 }; // Black queenside

                this->castling[from][to] = rights;
                this->castlingRookMoves[from][to] = rookMove;
            }
        }
    }

    // ----------------------------
    // Precompute all attacks
    // ----------------------------
    void AttackTable::preComputeAll() {
        preComputePawn(WHITE);
        preComputePawn(BLACK);
        preComputeKnight();
        preComputeSliding(BISHOP);
        preComputeSliding(ROOK);
        preComputeSliding(QUEEN);
        preComputeCastling();
        preComputeKing();
    }

    Bitboard AttackTable::getPawnAttacks(int color, int square)
    {
        return this->pawns[color][square];
    }

    Bitboard AttackTable::getKnightAttacks(int square)
    {
        return this->pieces[KNIGHT][square];
    }

    Bitboard AttackTable::getKingAttacks(int square)
    {
        return this->pieces[KING][square];
    }

    Bitboard AttackTable::getSlidingAttacks(int square, PieceType type)
    {
        return this->pieces[type][square];
    }

} // namespace PrecomputedTables