#include <search/ordering.hpp>
#include <search/config.hpp>

#include <core/board.hpp>

namespace Search::MoveOrdering {
    int Manager::getMvvLvaScore(Rune::Game& game, Move move)
    {
        int from = Helpers::getFrom(move);
        int to   = Helpers::getTo(move);

        Piece fPiece = game.boardGhost[from];
        Piece tPiece = game.boardGhost[to];

        int fType = Helpers::getType(fPiece);
        int tType = Helpers::getType(tPiece);

        return Config::mvvLvaScores[fType - 1][tType - 1];
    }

    bool Manager::predictCheck(Rune::Game& game, Move move)
    {
        int enemyKingSquare = Board::findKing(game, !game.turn);

        Bitboard newAttacks = game.attackWorker.getNewAttacksForMove(game, move);

        if ((newAttacks & (1ULL << enemyKingSquare)) != 0) return true;

        return false;
    }

    bool Manager::predictRecapture(Rune::Game& game, Move move)
    {
        int to = Helpers::getTo(move);

        Bitboard enemyAttacks = game.attackWorker.attackMapFull[!game.turn];

        if (enemyAttacks & (1ULL << to))
            // Opponent can recapture
            return true;
        
        return false;
    }

    int Manager::evaluateStaticExchange(Rune::Game& game, int to)
    {
        int gain[16]; // usually enough
        Piece captured = game.boardGhost[to];
        if (captured == EMPTY) return 0;

        gain[0] = game.evalWorker.pieceValues[Helpers::getType(captured)];

        Bitboard occupancy = game.occupancy[BOTH];
        Bitboard colorAttackers[2];
        colorAttackers[WHITE] = game.attackWorker.getAttackersForSquare(game, WHITE, to);
        colorAttackers[BLACK] = game.attackWorker.getAttackersForSquare(game, BLACK, to);

        int numCaptures = 0;
        int side = game.turn;

        while (colorAttackers[side])
        {
            int from = getLeastValuablePiece(game, colorAttackers[side]);
            Piece piece = game.boardGhost[from];
            numCaptures++;

            gain[numCaptures] = game.evalWorker.pieceValues[Helpers::getType(piece)] - gain[numCaptures - 1];

            if (std::max(-gain[numCaptures - 1], gain[numCaptures]) < 0)
                break;

            occupancy ^= (1ULL << from);
            colorAttackers[side] &= ~(1ULL << from); // remove this attacker
            side ^= 1;
        }

        int bestGain = gain[0];
        for (int i = 1; i <= numCaptures; i++)
            bestGain = std::max(-gain[i], bestGain);

        return bestGain;
    }

    int Manager::getLeastValuablePiece(Rune::Game& game, Bitboard options)
    {
        int bestOption = -1;
        int bestValue = Config::INF;

        while (options)
        {
            int square = Helpers::popLsb(options);

            Piece piece = game.boardGhost[square];

            int type = Helpers::getType(piece);
            int value = game.evalWorker.pieceValues[type];

            if (value < bestValue)
            {
                bestValue = value;
                bestOption = square;
            }
        }

        return bestOption;
    }

    void Manager::orderMoves(Rune::Game& game, Movegen::MoveList& movelist)
    {
        struct MoveScore {
            Move move;
            int score;
        };

        std::vector<MoveScore> scored;
        scored.reserve(movelist.size());

        for (int i = 0; i < movelist.size(); i++)
        {
            Move m = movelist[i];
            int score = 0;

            int from = Helpers::getFrom(m);
            int to   = Helpers::getTo(m);

            // ----------------------------
            // 1. Transposition Table move
            // ----------------------------
            if (m == this->ttMove)
                score += Config::SEARCH_MOVE_TT;

            // ----------------------------
            // 2. Captures: SEE + MVV-LVA
            // ----------------------------
            if (Helpers::isCapture(m))
            {
                // Static Exchange Evaluation: expected material gain
                int seeValue = evaluateStaticExchange(game, to);
                score += Config::SEARCH_MOVE_CAPTURE + seeValue;

                // MVV-LVA tie-breaker (optional small fraction)
                score += getMvvLvaScore(game, m) / 10;

                // Good / bad capture bias
                if (!predictRecapture(game, m))
                    score += Config::SEARCH_MOVE_CAPTURE_BIAS;  // good capture
                else
                    score -= Config::SEARCH_MOVE_CAPTURE_BIAS;  // bad capture
            }

            // ----------------------------
            // 3. Check bonus
            // ----------------------------
            if (predictCheck(game, m))
                score += Config::SEARCH_MOVE_CHECK;

            // ----------------------------
            // 4. Promotion bonus
            // ----------------------------
            if (Helpers::isPromo(m))
                score += Config::SEARCH_MOVE_PROMOTION;

            // ----------------------------
            // 5. Beta-cutoff history
            // ----------------------------
            if (game.config.search.doBetaCutoffHistory)
                score += tables.getBetaCutoffEntry(game.turn, from, to);

            // Store move + score
            scored.push_back({ m, score });
        }

        // ----------------------------
        // 6. Sort moves descending by score
        // ----------------------------
        std::sort(scored.begin(), scored.end(), [](const MoveScore &a, const MoveScore &b) {
            return a.score > b.score;
        });

        // ----------------------------
        // 7. Rewrite movelist in order
        // ----------------------------
        for (size_t i = 0; i < scored.size(); i++)
            movelist[i] = scored[i].move;
    }

    void Manager::requestMoves(Rune::Game& game, Movegen::MoveList& movelist, Types::MoveRequestType requestType)
    {
        game.movegenWorker.getLegalMoves(game, movelist, requestType == Types::QUIESCENSE);

        orderMoves(game, movelist);
    }

    void Manager::setTTMove(Move move)
    {
        this->ttMove = move;
    }
}