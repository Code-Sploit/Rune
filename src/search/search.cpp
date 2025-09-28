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
        if (bestMove != 0) {
            pv.clear();
            pv.push_back(bestMove);

            // Limit continuation length to avoid overflowing depth
            int maxLen = game.config.search.maximumQuiescenseDepth - depth;
            if ((int)bestChildPV.size() > maxLen)
                bestChildPV.resize(maxLen);

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

            if (game.repetitionTable.checkThreefold(game.zobristKey)) {
                eval = Config::DRAW_SCORE;
            } else {
                if (i == 0) {
                    // First move: full-window search
                    eval = -negamax(game, newDepth, -beta, -alpha, ply + 1, childPV);
                } else {
                    bool isQuietMove = !Helpers::isCapture(move) &&
                                    !Helpers::isPromo(move) &&
                                    !orderManager.predictCheck(game, move);

                    int reduction = 0;
                    if (depth >= 3 && i >= 4 && isQuietMove) {
                        reduction = 1;
                        if (depth >= 5 && i >= 10) reduction++;
                        if (depth >= 7 && i >= 15) reduction++;
                    }

                    // First try reduced-depth null-window search
                    eval = -negamax(game, newDepth - reduction, -alpha - 1, -alpha, ply + 1, childPV);

                    // If re-search is needed, reset PV and search again
                    if ((reduction > 0 && eval > alpha) || (eval > alpha && eval < beta)) {
                        childPV.clear();
                        eval = -negamax(game, newDepth, -beta, -alpha, ply + 1, childPV);
                    }
                }
            }

            Board::unmakeMove(game, MAKE_MOVE_FULL);

            // Only update best move and PV if strictly better
            if (eval > bestEval) {
                bestEval = eval;
                bestMove = move;
                bestChildPV = childPV;

                // Update alpha *after* bestEval is set
                if (eval > alpha) {
                    alpha = eval;
                    flag = TT_EXACT;
                }
            }

            if (alpha >= beta) {
                flag = TT_BETA;

                if (!Helpers::isCapture(move) && game.config.search.doBetaCutoffHistory)
                    orderManager.tables.addBetaCutoff(move, depth, game.turn);

                break;
            }
        }

        // Build PV once, at the end
        pv.clear();
        if (bestMove != 0) {
            pv.push_back(bestMove);

            // Limit continuation length: we searched depth-1 below this node
            int maxLen = depth - 1;
            if ((int)bestChildPV.size() > maxLen)
                bestChildPV.resize(maxLen);

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
        if (game.config.search.doOpeningBook)
            bookMove = OpeningBook::tryBookMove(game);

        if (bookMove && game.ply <= 12 && !game.outOfOpeningBook)
            return bookMove;
        else if (game.config.search.doOpeningBook)
            game.outOfOpeningBook = true;

        Move bestMoveSoFar = 0;
        Movegen::MoveList movelist;
        std::vector<Move> rootPV;
        int prevEval = 0;  // last iteration's score

        for (int depth = 1; depth <= initialDepth; depth++) {
            timerManager.checkTimer();
            if (timerManager.isTimeUp()) break;

            if (game.config.search.doBetaCutoffHistory)
                orderManager.tables.updateBetaCutoffHistory();

            Move bestThisDepth = 0;
            int evalThisDepth = -Config::INF;
            bool completed = true;

            timerManager.startNewDepth();

            // Probe TT for PV move
            Move tmpBestMove = 0;
            int tmpScore = 0;
            if (game.config.search.doTranspositions) {
                game.transpositionTable.probe(game.zobristKey, depth, -Config::INF, Config::INF, 0, tmpScore, tmpBestMove);
                orderManager.setTTMove(tmpBestMove);
            }

            orderManager.requestMoves(game, movelist, Types::NEGAMAX);
            if (movelist.size() == 1) return movelist[0];

            std::vector<Move> bestPV;

            // --- Aspiration window setup ---
            int alpha = -Config::INF;
            int beta  = Config::INF;
            if (depth > 1) {
                alpha = prevEval - Config::aspirationWindow;
                beta  = prevEval + Config::aspirationWindow;
            }

            int retries = 0;
            const int MAX_RETRIES = 3;
            bool reSearch = true;

            while (reSearch && retries < MAX_RETRIES) {
                retries++;
                reSearch = false;

                int windowLow  = alpha;
                int windowHigh = beta;

                evalThisDepth = -Config::INF;
                bestThisDepth = 0;
                bestPV.clear();

                for (int i = 0; i < movelist.size(); i++) {
                    timerManager.checkTimer();
                    if (timerManager.isTimeUp()) { completed = false; break; }

                    Move move = movelist[i];
                    Board::makeMove(game, move, MAKE_MOVE_FULL);

                    std::vector<Move> childPV;
                    int score;

                    if (i == 0) {
                        // full-window search for first move
                        score = -negamax(game, depth - 1, -beta, -alpha, 1, childPV);
                    } else {
                        // PVS null-window search
                        score = -negamax(game, depth - 1, -alpha - 1, -alpha, 1, childPV);
                        if (score > alpha && score < beta)
                            score = -negamax(game, depth - 1, -beta, -alpha, 1, childPV);
                    }

                    Board::unmakeMove(game, MAKE_MOVE_FULL);

                    if (score > evalThisDepth) {
                        evalThisDepth = score;
                        bestThisDepth = move;
                        bestPV.clear();
                        bestPV.push_back(move);
                        bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
                    }

                    if (evalThisDepth > alpha) alpha = evalThisDepth;
                    if (alpha >= beta) break; // beta cutoff
                }

                // --- Check aspiration window failure ---
                if (depth > 1) { // only apply to depth > 1
                    if (evalThisDepth <= windowLow) {
                        // fail-low → widen downwards
                        alpha = -Config::INF;
                        beta  = evalThisDepth + Config::aspirationWindowGrow;
                        reSearch = true;
                    } else if (evalThisDepth >= windowHigh) {
                        // fail-high → widen upwards
                        alpha = evalThisDepth - Config::aspirationWindowGrow;
                        beta  = Config::INF;
                        reSearch = true;
                    }
                }
            }

            rootPV = bestPV;
            game.pvLine.clear();
            for (Move m : rootPV)
                game.pvLine += Board::moveToString(m) + " ";

            timerManager.finishDepth();

            if (completed && game.config.search.doInfo) {
                bool isMate = (std::abs(evalThisDepth) > MATE_THRESHOLD);
                int score = evalThisDepth;
                if (isMate) {
                    int mateIn = (MATE_SCORE - std::abs(evalThisDepth) + 1) / 2;
                    mateIn = (mateIn == 0) ? 1 : mateIn;
                    if (evalThisDepth < 0) mateIn = -mateIn;
                    score = mateIn;
                }
                UCI::printSearchResult(depth, score, timerManager.getTimer(), isMate, game.pvLine);
            } else if (!completed) break;

            bestMoveSoFar = bestThisDepth;
            prevEval = evalThisDepth;
        }

        if (game.config.search.doInfo)
            UCI::debug(__FILE__, "timeUsed=%.0f ms\n", timerManager.getElapsedTime());

        return bestMoveSoFar;
    }
} // namespace Search