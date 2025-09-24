#pragma once

#include <tables/constants.hpp>
#include <tables/table.hpp>   // PrecomputedTables::AttackTable
#include <tables/helpers.hpp>

namespace Rune {
    class State;
    class Game; // forward declaration
}

namespace Attack {
    class Worker {
        public:
            Bitboard attackMap[2][64];
            Bitboard attackMapFull[2];
            Bitboard attackMapIncludes;

            void generatePawns(Rune::Game& game, PieceColor color);
            void generateKnights(Rune::Game& game, PieceColor color);
            void generateKing(Rune::Game& game, PieceColor color);
            void generateSliding(Rune::Game& game, PieceColor color, PieceType type);

            void generateTable(Rune::Game& game, int side);
            void printTable(Rune::Game& game, int side);

            void generateAll(Rune::Game& game);
            void update(Rune::Game& game, Move move);
            void restore(Rune::Game& game, const Rune::State& s);

            bool isSquareAttackedBy(int square, int color);
            
            Bitboard getNewAttacksForMove(Rune::Game& game, Move move);
            Bitboard getAttackersForSquare(Rune::Game& game, int color, int square);
            Bitboard getAttackersForZone(Rune::Game& game, Bitboard zone, int color, int type);

            PrecomputedTables::AttackTable preComputed;
    };
} // namespace Attack