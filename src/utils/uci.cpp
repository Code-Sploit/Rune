#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cstdarg>

#include <tables/zobrist.hpp>
#include <utils/uci.hpp>

#include <core/board.hpp>
#include <core/perft.hpp>

namespace Rune {
    class Game;
}

namespace UCI {
    void debug(const char* file, const char* format, ...)
    {
        std::string filename(file);\

        size_t lastSlash = filename.find_last_of("/\\");

        if (lastSlash != std::string::npos)
            filename = filename.substr(lastSlash + 1);
        
        size_t dotPos = filename.find_last_of('.');

        if (dotPos != std::string::npos)
            filename = filename.substr(0, dotPos);
        
        if (!filename.empty() && std::islower(filename[0]))
            filename[0] = std::toupper(filename[0]);
        
        va_list args;

        va_start(args, format);

        std::cerr << "info string " << filename << " ";

        vfprintf(stderr, format, args);

        va_end(args);

        std::cerr << std::endl;
    }

    void printSearchResult(int depth, int score, int timeMs, bool isMate, std::string pvCurrent)
    {
        std::cout << "info depth " << depth << " score";

        if (isMate)
            std::cout << " mate " << score << " ";
        else
            std::cout << " cp " << score << " ";
        
        std::cout << "time " << static_cast<int>(timeMs) << " ";
        std::cout << "pv " << pvCurrent << std::endl;
    }

    void uciLoop(Rune::Game& game)
    {
        char input[4096];

        while (fgets(input, sizeof(input), stdin))
        {
            // Strip trailing newline if present
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "uci") == 0)
            {
                std::cout << "id name " << uciVersion << std::endl;
                std::cout << "id author " << uciAuthor << std::endl;
                std::cout << "uciok" << std::endl;

                fflush(stdout);
            }
            else if (strcmp(input, "ucinewgame") == 0)
            {
                game.historyCount = 0;
                game.zobristKey = 0ULL;
                game.isFirstLoad = 1;

                game.repetitionTable.clear();
                game.transpositionTable.clear();
            }
            else if (strcmp(input, "isready") == 0)
            {
                std::cout << "readyok" << std::endl;

                fflush(stdout);
            }
            else if (strncmp(input, "setoption", 9) == 0)
            {
                char *namePos = strstr(input, "name");
                char *valuePos = strstr(input, "value");

                if (namePos && valuePos)
                {
                    namePos += 5; // skip "name "
                    valuePos += 6; // skip "value "

                    std::string nameStr(namePos, valuePos - namePos - 6);
                    std::string valueStr(valuePos);

                    // Trim trailing/leading spaces
                    auto trim = [](std::string &s) {
                        size_t start = s.find_first_not_of(" ");
                        size_t end   = s.find_last_not_of(" ");

                        if (start == std::string::npos) { s.clear(); return; }
                        
                        s = s.substr(start, end - start + 1);
                    };
                    
                    trim(nameStr);
                    trim(valueStr);

                    if (Config::setOption(game, nameStr.c_str(), valueStr.c_str()) == 1)
                    {
                        std::cout << "info string Unknown option: " << nameStr << std::endl;
                    }
                }
                else
                {
                    std::cout << "info string Invalid setoption command" << std::endl;
                }

                fflush(stdout);
            }
            else if (strncmp(input, "position", 8) == 0)
            {
                char *ptr = input + 9;

                game.repetitionTable.clear();

                if (strncmp(ptr, "startpos", 8) == 0)
                {
                    Board::loadFen(game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

                    ptr += 8;
                }
                else if (strncmp(ptr, "fen", 3) == 0)
                {
                    ptr += 4;

                    char fen[4096];
                    
                    strncpy(fen, ptr, sizeof(fen) - 1);
                    
                    fen[sizeof(fen) - 1] = '\0';

                    char *moves_pos = strstr(fen, " moves");
                    
                    if (moves_pos) *moves_pos = '\0';

                    Board::loadFen(game, fen);
                }

                // Parse moves after "moves"
                char *moves = strstr(ptr, "moves");

                if (moves)
                {
                    moves += 6;

                    while (*moves)
                    {
                        char move_str[6];

                        if (sscanf(moves, "%5s", move_str) != 1) break;

                        Move move = Board::parseMove(game, move_str);
                        
                        Board::makeMove(game, move, MAKE_MOVE_FULL);

                        moves += strcspn(moves, " ");

                        while (*moves == ' ') moves++;
                    }
                }

                game.attackWorker.generateAll(game);

                Zobrist::updateBoard(game);
                
                fflush(stdout);
            }
            else if (strncmp(input, "go perft ", 9) == 0)
            {
                Perft::root(game, std::atoi(input + 9));
            }
            else if (strncmp(input, "go perfttest", 12) == 0)
            {
                Perft::runTests(game);
            }
            else if (strncmp(input, "go", 2) == 0)
            {
                char *ptr = input + 2;
                int wtime = -1, btime = -1, winc = 0, binc = 0;
                int movestogo = 30, depth = -1, movetime = -1;

                while (*ptr)
                {
                    if (strncmp(ptr, "wtime", 5) == 0) { ptr += 5; while (*ptr == ' ') ptr++; wtime = atoi(ptr); }
                    else if (strncmp(ptr, "btime", 5) == 0) { ptr += 5; while (*ptr == ' ') ptr++; btime = atoi(ptr); }
                    else if (strncmp(ptr, "winc", 4) == 0)  { ptr += 4; while (*ptr == ' ') ptr++; winc = atoi(ptr); }
                    else if (strncmp(ptr, "binc", 4) == 0)  { ptr += 4; while (*ptr == ' ') ptr++; binc = atoi(ptr); }
                    else if (strncmp(ptr, "movestogo", 9) == 0) { ptr += 9; while (*ptr == ' ') ptr++; movestogo = atoi(ptr); }
                    else if (strncmp(ptr, "depth", 5) == 0) { ptr += 5; while (*ptr == ' ') ptr++; depth = atoi(ptr); }
                    else if (strncmp(ptr, "movetime", 8) == 0) { ptr += 8; while (*ptr == ' ') ptr++; movetime = atoi(ptr); }

                    while (*ptr && *ptr != ' ') ptr++;
                    while (*ptr == ' ') ptr++;
                }

                Move best_move;

                if (movetime > 0)
                    best_move = game.searchWorker.searchPosition(game, game.config.search.maximumDepth, movetime);
                else if (depth > 0)
                    best_move = game.searchWorker.searchPosition(game, std::min(game.config.search.maximumDepth, depth), Search::Config::maximumSearchTime);
                else if (wtime > 0 && btime > 0)
                {
                    int time_left = (game.turn == WHITE) ? wtime : btime;
                    int increment = (game.turn == WHITE) ? winc : binc;
                    if (movestogo <= 0) movestogo = 30;

                    int base_time = time_left / movestogo;
                    int think_time = base_time + increment / 2;

                    int max_time = time_left * 60 / 100;
                    if (think_time > max_time) think_time = max_time;
                    if (think_time < 10) think_time = 10;
                    if (time_left < 60000) { think_time = time_left / 10; if (think_time < 5) think_time = 5; }

                    best_move = game.searchWorker.searchPosition(game, 64, think_time);
                }
                else
                {
                    best_move = game.searchWorker.searchPosition(game, std::min(game.config.search.initialDepth, game.config.search.maximumDepth), Search::Config::maximumSearchTime);
                }

                std::cout << "bestmove " << ((best_move == 0) ? "(none)" : Board::moveToString(best_move)) << std::endl;

                fflush(stdout);
            }
            else if (strcmp(input, "config") == 0)
            {
                printf("\n=== Current Configuration ===\n");

                // MoveGen options
                printf("MoveGen:\n");
                printf("  doLegalMoveFiltering: %d\n", game.config.moveGen.doLegalMoveFiltering);
                printf("  doOnlyCaptures:       %d\n", game.config.moveGen.doOnlyCaptures);

                // Eval options
                printf("\nEval:\n");
                printf("  doMaterial:             %d\n", game.config.eval.doMaterial);
                printf("  doPieceSquares:         %d\n", game.config.eval.doPieceSquares);
                printf("  doMobility:             %d\n", game.config.eval.doMobility);
                printf("  doBishopPair:           %d\n", game.config.eval.doBishopPair);
                printf("  doPawnStructure:        %d\n", game.config.eval.doPawnStructure);
                printf("  doKingSafety:           %d\n", game.config.eval.doKingSafety);

                // Search options
                printf("\nSearch:\n");
                printf("  doQuiescense:           %d\n", game.config.search.doQuiescense);
                printf("  doTranspositions:       %d\n", game.config.search.doTranspositions);
                printf("  doBetaCutoffHistory:    %d\n", game.config.search.doBetaCutoffHistory);
                printf("  doInfo:                 %d\n", game.config.search.doInfo);
                printf("  initialDepth:           %d\n", game.config.search.initialDepth);
                printf("  maximumDepth:           %d\n", game.config.search.maximumDepth);
                printf("  maximumQuiescenseDepth: %d\n", game.config.search.maximumQuiescenseDepth);

                printf("===========================\n");
                fflush(stdout);
            }
            else if (strcmp(input, "quit") == 0 || strcmp(input, "stop") == 0)
            {
                break;
            }
            else if (strcmp(input, "display") == 0)
            {
                Board::print(game);
                fflush(stdout);
            }
            else if (strcmp(input, "atw") == 0) game.attackWorker.printTable(game, WHITE);
            else if (strcmp(input, "atb") == 0) game.attackWorker.printTable(game, BLACK);
            else if (strcmp(input, "eval") == 0) printf("info string Eval: %d\n", game.evalWorker.evaluate(game));
        }
    }
}