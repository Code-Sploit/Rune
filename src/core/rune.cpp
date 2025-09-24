#include <core/rune.hpp>
#include <core/eval.hpp>
#include <search/search.hpp>
#include <tables/zobrist.hpp>
#include <cstdlib>
#include <cstdio>

namespace Rune {
    Game::Game()
    : board{}, occupancy{}, boardGhost{},
      turn(WHITE),
      enpassantSquare(-1),
      castlingRights(CASTLING_ALL),
      ply(0),
      hasCastled{false, false},
      movelist(),
      transpositionTable(),
      repetitionTable(),
      attackWorker(),
      movegenWorker(),
      evalWorker(),
      searchWorker(),
      config(),              // default construct nested config
      historyCount(0),
      isFirstLoad(1),
      history(std::make_unique<State[]>(HISTORY_SIZE)),
      outOfOpeningBook(false),
      zobristKey(0ULL),
      pvLine()
    {
        // Initialize attack worker precomputed tables
        attackWorker.preComputed.preComputeAll();

        // Ensure attack maps are cleared
        for (int c = 0; c < 2; ++c) {
            attackWorker.attackMapFull[c] = 0ULL;
            for (int s = 0; s < 64; ++s)
                attackWorker.attackMap[c][s] = 0ULL;
        }

        // Initialize zobrist hasher
        Zobrist::init();
    }

    Game::~Game() {
        // All members RAII-managed, nothing else to delete
    }
} // namespace Rune