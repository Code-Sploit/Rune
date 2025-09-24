#pragma once

#include <tables/constants.hpp>
#include <search/types.hpp>
#include <search/tables.hpp>
#include <core/movegen.hpp>

namespace Rune {
    class Game;
}

namespace Search::MoveOrdering {
    class Manager {
        private:
            Move ttMove;
            Move pvMove;

        public:
            Tables::Manager tables;

            int getMvvLvaScore(Rune::Game& game, Move move);
            
            bool predictCheck(Rune::Game& game, Move move);
            bool predictRecapture(Rune::Game& game, Move move);
            
            int evaluateStaticExchange(Rune::Game& game, int to);
            int getLeastValuablePiece(Rune::Game& game, Bitboard options);

            // --- Move ordering & requests ---
            void orderMoves(Rune::Game& game, Movegen::MoveList& movelist);
            void requestMoves(Rune::Game& game, Movegen::MoveList& movelist, Types::MoveRequestType requestType);

            void setTTMove(Move move);
    };
}