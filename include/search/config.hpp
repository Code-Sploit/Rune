#pragma once

namespace Search::Config {
    // --- Move ordering constants ---
    const int SEARCH_MOVE_TT           = 1000000;
    const int SEARCH_MOVE_CAPTURE      = 900000;
    const int SEARCH_MOVE_CHECK        = 800000;
    const int SEARCH_MOVE_CAPTURE_BIAS = 25000;
    const int SEARCH_MOVE_PROMOTION    = 80000;

    const int DRAW_SCORE = 0;

    const int nullMovePruneReduction = 2;

    // --- History heuristic ---
    const int HISTORY_MAX       = 160000;
    const int HISTORY_SCALE_FAC = 8;

    // --- MVV/LVA table ---
    const int mvvLvaScores[5][5] = {
        {900, 700, 680, 500, 100},
        {2700, 2400, 2380, 2200, 1800},
        {2900, 2600, 2580, 2400, 2000},
        {4900, 4600, 4580, 4400, 4000},
        {8900, 8600, 8580, 8400, 8000}
    };

    // --- Public constants ---
    const int maximumSearchTime = 1000000;
    const int searchThinkTimeMargin = 10;

    const int INF = 1000000;
}