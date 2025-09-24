#include <core/eval.hpp>
#include <core/board.hpp>
#include <core/rune.hpp>
#include <tables/magic.hpp>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>

namespace Rune { class Game; }

namespace Evaluation
{
    Worker::GamePhase Worker::getGamePhase(Rune::Game& game)
    {
        int scores[3] = {0,0,0}; // OPENING, MIDDLEGAME, ENDGAME

        if (Board::hasFullMaterial(game, WHITE) && Board::hasFullMaterial(game, BLACK))
            scores[OPENING] += 5;

        if (Board::pawnChainsLocked(game))
            scores[OPENING] += 3;

        if (Board::countPieces(game, QUEEN) >= 1 && Board::totalMaterial(game) < 40)
            scores[MIDDLEGAME] += 4;

        if (Board::countPieces(game, ROOK) + Board::countPieces(game, QUEEN) >= 2)
            scores[MIDDLEGAME] += 2;

        if (!Board::hasNonPawnMaterial(game, WHITE) && !Board::hasNonPawnMaterial(game, BLACK))
            scores[ENDGAME] += 5;

        if (!Board::hasPiece(game, QUEEN, WHITE) && !Board::hasPiece(game, QUEEN, BLACK))
            scores[ENDGAME] += 3;

        int whiteKingRank = Helpers::rankOf(Board::findKing(game, WHITE));
        int blackKingRank = Helpers::rankOf(Board::findKing(game, BLACK));

        if (whiteKingRank > 4 || blackKingRank < 3)
            scores[ENDGAME] += 2;

        int index = Helpers::findLargestOfThree(scores[OPENING], scores[MIDDLEGAME], scores[ENDGAME]);

        return static_cast<GamePhase>(index);
    }

    int Worker::getPSTFor(PieceType type, int square, GamePhase phase)
    {
        const PieceSquareTable& table = pieceSquareTables[type-1];

        switch (phase)
        {
            case OPENING:     return table.openingValue[square];
            case MIDDLEGAME:  return table.middlegameValue[square];
            case ENDGAME:     return table.endgameValue[square];
            default:          return 0;
        }
    }

    int Worker::getMobilityScoreFor(Rune::Game& game, PieceType type, int square)
    {
        int score = 0;
        int phasePerspective = (getGamePhase(game) == ENDGAME) ? -1 : 1;

        switch (type)
        {
            case PAWN:   score += EVAL_PAWN_MOBILITY_SCORE; break;
            case KNIGHT: score += __builtin_popcountll(game.attackWorker.preComputed.getKnightAttacks(square)); break;
            case BISHOP: score += __builtin_popcountll(Magic::getBishopAttacks(square, game.occupancy[BOTH])); break;
            case ROOK:   score += __builtin_popcountll(Magic::getRookAttacks(square, game.occupancy[BOTH])); break;
            case QUEEN:  score += __builtin_popcountll(Magic::getQueenAttacks(square, game.occupancy[BOTH])); break;
            case KING:   score += __builtin_popcountll(game.attackWorker.preComputed.getKingAttacks(square)) * phasePerspective; break;
            default: break;
        }

        return score;
    }

    Bitboard Worker::getPassedPawnMask(int square, int color)
    {
        int rank = Helpers::rankOf(square);
        int file = Helpers::fileOf(square);
        int realRank = (color == WHITE) ? rank : (7 - rank);

        Bitboard maxValue = ~0ULL;
        Bitboard forwardMask = (color == WHITE) ? maxValue << (8*(realRank+1)) : maxValue >> (8*(realRank+1));
        Bitboard surroundingMask = Rune::FILE_MASKS[std::max(0,file-1)] | Rune::FILE_MASKS[file] | Rune::FILE_MASKS[std::min(7,file+1)];

        return forwardMask & surroundingMask;
    }

    Bitboard Worker::getPawnShield(int kingSquare, int side)
    {
        int file = Helpers::fileOf(kingSquare);
        int rank = Helpers::rankOf(kingSquare);

        int left = std::max(0,file-1);
        int right = std::min(7,file+1);

        Bitboard fileMask = Rune::FILE_MASKS[file] | Rune::FILE_MASKS[left] | Rune::FILE_MASKS[right];
        Bitboard rankMask = 0ULL;

        if (side == WHITE)
        {
            for (int r = rank; r <= std::min(7, rank + 2); r++)
                rankMask |= 0xFFULL << (r * 8);
        }
        else
        {
            for (int r = rank; r >= std::max(0, rank - 2); r--)
                rankMask |= 0xFFULL << (r * 8);
        }

        return fileMask & rankMask;
    }

    Bitboard Worker::getKingZone(Rune::Game& game, int kingSquare)
    {
        Bitboard zone = game.attackWorker.preComputed.getKingAttacks(kingSquare);
        zone |= 1ULL << kingSquare;
        return zone;
    }

    void Worker::modulePawnStructure(Rune::Game& game)
    {
        int moduleEval = 0;
        int passedBonuses[8] = {0, 15, 15, 30, 40, 60, 90, 0};

        for (int color = WHITE; color <= BLACK; color++)
        {
            Bitboard friendly = game.board[color][PAWN];
            Bitboard enemy = game.board[!color][PAWN];
            int perspective = (color == WHITE) ? 1 : -1;

            while (friendly)
            {
                int sq = Helpers::popLsb(friendly);
                int realRank = (color==WHITE) ? Helpers::rankOf(sq) : 7 - Helpers::rankOf(sq);

                Bitboard passedMask = getPassedPawnMask(sq, color);

                if ((passedMask & enemy) == 0)
                    moduleEval += passedBonuses[realRank] * perspective;
            }
        }

        this->eval += moduleEval;
    }

    void Worker::moduleMaterial(Rune::Game& game)
    {
        int moduleEval = 0;

        for (int color = WHITE; color <= BLACK; color++)
        {
            int perspective = (color == WHITE) ? 1 : -1;

            moduleEval += perspective * pieceValues[PAWN]   * __builtin_popcountll(game.board[color][PAWN]);
            moduleEval += perspective * pieceValues[KNIGHT] * __builtin_popcountll(game.board[color][KNIGHT]);
            moduleEval += perspective * pieceValues[BISHOP] * __builtin_popcountll(game.board[color][BISHOP]);
            moduleEval += perspective * pieceValues[ROOK]   * __builtin_popcountll(game.board[color][ROOK]);
            moduleEval += perspective * pieceValues[QUEEN]  * __builtin_popcountll(game.board[color][QUEEN]);
        }

        this->eval += moduleEval;
    }

    void Worker::modulePST(Rune::Game& game)
    {
        int moduleEval = 0;
        GamePhase phase = getGamePhase(game);
        Bitboard occ = game.occupancy[BOTH];

        while (occ)
        {
            int sq = Helpers::popLsb(occ);

            Piece p = game.boardGhost[sq];
            
            int type = Helpers::getType(p);
            int color = Helpers::getColor(p);
            int perspective = (color == WHITE) ? 1 : -1;

            int pstSq = (color == BLACK) ? sq : mirror[sq];
            
            moduleEval += getPSTFor(type, pstSq, phase) * perspective;
        }

        this->eval += moduleEval;
    }

    void Worker::moduleMobility(Rune::Game& game)
    {
        int moduleEval = 0;

        Bitboard occ = game.occupancy[BOTH];

        while (occ)
        {
            int sq = Helpers::popLsb(occ);
            
            Piece p = game.boardGhost[sq];
            
            int type = Helpers::getType(p);
            int color = Helpers::getColor(p);
            int perspective = (color == WHITE) ? 1 : -1;

            moduleEval += getMobilityScoreFor(game, type, sq) * perspective;
        }

        this->eval += moduleEval;
    }

    void Worker::modulePiecePairs(Rune::Game& game)
    {
        int moduleEval = 0;

        if (__builtin_popcountll(game.board[WHITE][BISHOP]) >= 2)
            moduleEval += EVAL_HAS_BISHOP_PAIR;

        if (__builtin_popcountll(game.board[BLACK][BISHOP]) >= 2)
            moduleEval -= EVAL_HAS_BISHOP_PAIR;

        int scaled = moduleEval;

        GamePhase phase = getGamePhase(game);

        if (phase == OPENING || phase == ENDGAME) scaled = int(scaled * 0.5);

        this->eval += scaled;
    }

    void Worker::moduleKingSafety(Rune::Game& game)
    {
        int kingSafety[2] = {0, 0};

        for (int color = WHITE; color <= BLACK; color++)
        {
            int sq = Board::findKing(game, color);

            Bitboard zone = getKingZone(game, sq);
            
            int safety = 0;

            if (game.hasCastled[color])
            {
                Bitboard shield = getPawnShield(sq, color);
                
                int count = __builtin_popcountll(shield & game.board[color][PAWN]);

                if (count < 3) safety -= EVAL_KING_SAFETY_MISSING_PAWN * (3 - count);
                if (count == 3) safety += EVAL_KING_SAFETY_FULL_SHIELD;
            }

            int file = Helpers::fileOf(sq);
            
            if (Board::isFileOpen(game ,file)) safety -= EVAL_KING_SAFETY_OPEN_FILE;
            else if (Board::isFileSemiOpen(game, file, color)) safety -= EVAL_KING_SAFETY_SEMI_OPEN_FILE;

            int attackScore = 0;

            for (int piece = PAWN; piece <= QUEEN; piece++)
            {
                Bitboard attackers = game.attackWorker.getAttackersForZone(game, zone, !color, piece);

                attackScore += kingSafetyPieceDanger[piece] * __builtin_popcountll(attackers);
            }

            int danger = std::min(attackScore * attackScore / 4, 200);

            safety -= danger;

            if (!game.hasCastled[color] && game.ply > 20)
            {
                int rank = Helpers::rankOf(sq);

                if (rank > 1 && rank < 6) safety -= EVAL_KING_SAFETY_CENTRAL_KING;
            }

            kingSafety[color] = safety;
        }

        int phase = Board::getPhase(game);
        int relativeSafety = kingSafety[WHITE] - kingSafety[BLACK];

        this->eval += relativeSafety * phase / 24;
    }

    int Worker::evaluate(Rune::Game& game)
    {
        this->eval = 0;

        if (game.config.eval.doMaterial) moduleMaterial(game);
        if (game.config.eval.doPieceSquares) modulePST(game);
        if (game.config.eval.doMobility) moduleMobility(game);
        if (game.config.eval.doBishopPair) modulePiecePairs(game);
        if (game.config.eval.doPawnStructure) modulePawnStructure(game);
        if (game.config.eval.doKingSafety) moduleKingSafety(game);

        return (game.turn==WHITE) ? this->eval : -this->eval;
    }
}