#include <search/search.hpp>
#include <storage/transposition.hpp>
#include <storage/repetition.hpp>
#include <storage/openingbook.hpp>
#include <core/movegen.hpp>
#include <core/board.hpp>
#include <core/rune.hpp>
#include <core/eval.hpp>
#include <utils/uci.hpp>
#include <tables/magic.hpp>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <vector>

namespace Rune {
    class Game;
}

namespace Search {
    bool Worker::isNullMovePruneSafe(Rune::Game& game, Movegen::MoveList& movelist)
    {
        // Disable NMP when in check
        bool inCheck = Board::isKingInCheck(game, game.turn);

        if (inCheck) return false;

        // Disable NMP when we are in late endgame positions or we dont have a lot of moves
        bool hasEnoughMoves = movelist.size() > 10;

        if (!hasEnoughMoves) return false;

        // Disable NMP when we only have pawns & kings
        bool hasOnlyPawnKing = ((Board::countPieces(game, KNIGHT) + Board::countPieces(game, BISHOP) + Board::countPieces(game, ROOK) + Board::countPieces(game, QUEEN)) == 0);

        if (hasOnlyPawnKing) return false;

        // If we pass everything we enable NMP
        return true;
    }

    // -------------------------
    // Quiescence search
    // -------------------------
    int Worker::quiescense(Rune::Game& game, int depth, int alpha, int beta, int ply, std::vector<Move>& pv)
    {
        if (depth >= game.config.search.maximumQuiescenseDepth)
            return game.evalWorker.evaluate(game);

        int standPat = game.evalWorker.evaluate(game);

        // PV for stand-pat
        pv.clear();

        // Alpha-beta stand pat checks
        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;

        Movegen::MoveList movelist;

        orderManager.requestMoves(game, movelist, Types::QUIESCENSE);

        std::vector<Move> bestChildPV;
        Move bestMove = 0;
        int bestScore = standPat;

        for (int i = 0; i < movelist.size(); i++)
        {
            timerManager.checkTimer();

            if (timerManager.isTimeUp()) break;

            Move move = movelist[i];

            Board::makeMove(game, move, MAKE_MOVE_LIGHT);
            std::vector<Move> childPV;

            int score = -quiescense(game, depth + 1, -beta, -alpha, ply + 1, childPV);

            Board::unmakeMove(game, MAKE_MOVE_LIGHT);

            if (score >= beta) 
            {
                // Beta cutoff: include the move in PV for info
                pv.clear();
                pv.push_back(move);
                return beta;
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move;
                bestChildPV = childPV;
                alpha = score;
            }
        }

        // Construct PV if a best move exists
        if (bestMove != 0)
        {
            pv.clear();
            pv.push_back(bestMove);
            pv.insert(pv.end(), bestChildPV.begin(), bestChildPV.end());
        }

        return bestScore;
    }

    // -------------------------
    // Negamax
    // -------------------------
    int Worker::negamax(Rune::Game& game, int depth, int alpha, int beta, int ply, std::vector<Move>& pv)
    {
        if (depth == 0)
        {
            int score = (game.config.search.doQuiescense) ? quiescense(game, 0, alpha, beta, ply + 1, pv) : game.evalWorker.evaluate(game);

            // Mate distance pruning
            if (score > MATE_THRESHOLD) score -= ply;
            if (score < -MATE_THRESHOLD) score += ply;
            return score;
        }

        ZobristHash key = game.zobristKey;

        Move ttMove = 0;
        int ttScore = 0;

        if (game.config.search.doTranspositions &&
            game.transpositionTable.probe(key, depth, alpha, beta, ply, ttScore, ttMove))
        {
            return ttScore;
        }

        // Store TT move so requestMoves can reorder
        orderManager.setTTMove(ttMove);

        Movegen::MoveList movelist;
        orderManager.requestMoves(game, movelist, Types::NEGAMAX);

        // Null-move pruning
        if (depth >= Config::nullMovePruneReduction + 1 && isNullMovePruneSafe(game, movelist))
        {
            Board::makeNullMove(game);
            int score = -negamax(game,
                                std::max(depth - 1 - Config::nullMovePruneReduction, 0),
                                -beta, -beta + 1, ply + 1, pv);
            Board::unmakeNullMove(game);

            if (score >= beta)
                return beta;
        }

        // No legal moves → mate or stalemate
        if (movelist.size() == 0)
            return Board::isKingInCheck(game, game.turn) ? -MATE_SCORE + ply : 0;

        int alphaOriginal = alpha;
        int bestEval = -Config::INF;
        int flag = TT_ALPHA;

        Move bestMove = 0;

        std::vector<Move> bestChildPV;

        for (int i = 0; i < movelist.size(); i++)
        {
            timerManager.checkTimer();
            if (timerManager.isTimeUp()) break;

            Move move = movelist[i];
            Board::makeMove(game, move, MAKE_MOVE_FULL);
            
            std::vector<Move> childPV;

            int eval = 0;
            int newDepth = depth - 1;

            if (game.repetitionTable.checkThreefold(game.zobristKey))
            {
                eval = Config::DRAW_SCORE;
            }
            else
            {
                if (i == 0)
                {
                    // First move: full-window search
                    eval = -negamax(game, newDepth, -beta, -alpha, ply + 1, childPV);
                }
                else
                {
                    bool isQuietMove = !Helpers::isCapture(move) &&
                                    !Helpers::isPromo(move) &&
                                    !orderManager.predictCheck(game, move);

                    int reduction = 0;

                    if (depth >= 3 && i >= 4 && isQuietMove)
                    {
                        reduction = 1;
                        if (depth >= 5 && i >= 10) reduction++;
                        if (depth >= 7 && i >= 15) reduction++;
                    }

                    // First try reduced depth null-window search (LMR + PVS combined)
                    eval = -negamax(game, newDepth - reduction, -alpha - 1, -alpha, ply + 1, childPV);

                    // Re-search if needed
                    if ((reduction > 0 && eval > alpha) || (eval > alpha && eval < beta))
                    {
                        eval = -negamax(game, newDepth, -beta, -alpha, ply + 1, childPV);
                    }
                }
            }

            Board::unmakeMove(game, MAKE_MOVE_FULL);

            if (eval > bestEval)
            {
                bestEval = eval;
                bestMove = move;
                bestChildPV = childPV;
            }

            if (eval > alpha)
            {
                alpha = eval;
                flag = TT_EXACT; // we found a better move
            }

            if (alpha >= beta)
            {
                flag = TT_BETA;

                if (!Helpers::isCapture(move) && game.config.search.doBetaCutoffHistory)
                    orderManager.tables.addBetaCutoff(move, depth, game.turn);
                
                break;
            }
        }

        pv.clear();

        if (bestMove != 0)
        {
            pv.push_back(bestMove);
            pv.insert(pv.end(), bestChildPV.begin(), bestChildPV.end());
        }

        if (game.config.search.doTranspositions)
        {
            if (bestEval <= alphaOriginal) flag = TT_ALPHA;
            else if (bestEval >= beta) flag = TT_BETA;
            else flag = TT_EXACT;

            game.transpositionTable.store(key, depth, bestEval, flag, bestMove, ply);
        }

        return bestEval;
    }

    // -------------------------
    // Entry point
    // -------------------------
    Move Worker::searchPosition(Rune::Game& game, int initialDepth, int thinkTimeMs)
    {
        timerManager.setTimer(thinkTimeMs);
        
        if (game.config.search.doInfo)
            UCI::debug(__FILE__, "start with initialDepth=%d thinkTime=%d ms", initialDepth, thinkTimeMs);
        
        Move bookMove = 0;
        
        if (game.config.search.doOpeningBook) bookMove = OpeningBook::tryBookMove(game);

        if (bookMove && game.ply <= 12 && !game.outOfOpeningBook)
        {
            return bookMove;
        }
        else if (game.config.search.doOpeningBook)
        {
            game.outOfOpeningBook = true;
        }

        Move bestMoveSoFar = 0;
        Movegen::MoveList movelist;
        std::vector<Move> rootPV;

        for (int depth = 1; depth <= initialDepth; depth++)
        {
            timerManager.checkTimer();
            
            if (timerManager.isTimeUp()) break;

            if (game.config.search.doBetaCutoffHistory) orderManager.tables.updateBetaCutoffHistory();

            Move bestThisDepth = 0;
            int evalThisDepth = -Config::INF;
            bool completed = true;

            timerManager.startNewDepth();

            // Probe TT for PV move to reorder
            Move tmpBestMove = 0;
            int tmpScore = 0;
            if (game.config.search.doTranspositions) {
                game.transpositionTable.probe(game.zobristKey, depth, -Config::INF, Config::INF, 0, tmpScore, tmpBestMove);
                orderManager.setTTMove(tmpBestMove);
            }

            orderManager.requestMoves(game, movelist, Types::NEGAMAX);

            std::vector<Move> bestPV;

            if (movelist.size() == 1) return movelist[0];

            for (int i = 0; i < movelist.size(); i++)
            {
                timerManager.checkTimer();

                if (timerManager.isTimeUp()) { completed = false; break; }

                Move move = movelist[i];
                Board::makeMove(game, move, MAKE_MOVE_FULL);

                std::vector<Move> childPV;

                int score;
                if (i == 0) {
                    // First move: full window
                    score = -negamax(game, depth - 1, -Config::INF, Config::INF, 1, childPV);
                } else {
                    // PVS search
                    score = -negamax(game, depth - 1, -evalThisDepth - 1, -evalThisDepth, 1, childPV);
                    if (score > evalThisDepth) {
                        score = -negamax(game, depth - 1, -Config::INF, Config::INF, 1, childPV);
                    }
                }

                Board::unmakeMove(game, MAKE_MOVE_FULL);

                if (score > evalThisDepth) {
                    evalThisDepth = score;
                    bestThisDepth = move;
                    bestPV.clear();
                    bestPV.push_back(move);
                    bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
                }
            }

            rootPV = bestPV;

            game.pvLine.clear();

            for (Move m : rootPV)
            {
                game.pvLine += Board::moveToString(m) + " ";
            }

            timerManager.finishDepth();

            // Print info
            if (completed && game.config.search.doInfo)
            {
                bool isMate = (std::abs(evalThisDepth) > MATE_THRESHOLD);
                int score = evalThisDepth;

                if (isMate)
                {
                    int mateIn = (MATE_SCORE - std::abs(evalThisDepth) + 1) / 2;
                    mateIn = (mateIn == 0) ? 1 : mateIn;
                    if (evalThisDepth < 0) mateIn = -mateIn;
                    score = mateIn;
                }

                UCI::printSearchResult(depth, score, timerManager.getTimer(), isMate, game.pvLine);
            }
            else if (!completed) break;

            bestMoveSoFar = bestThisDepth;
        }

        if (game.config.search.doInfo)
            UCI::debug(__FILE__, "timeUsed=%.0f ms\n", timerManager.getElapsedTime());

        return bestMoveSoFar;
    }
} // namespace Search