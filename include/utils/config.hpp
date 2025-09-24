#pragma once

#include <cstddef>  // for size_t
#include <string>

namespace Rune {
    class Game; // Forward declaration
}

namespace Config {

    // ----------------------------
    // Move Generation Options
    // ----------------------------
    struct MoveGenOptions {
        bool doLegalMoveFiltering = true;
        bool doOnlyCaptures = false;
    };

    // ----------------------------
    // Evaluation Options
    // ----------------------------
    struct EvalOptions {
        bool doMaterial = true;
        bool doPieceSquares = true;
        bool doMobility = true;
        bool doBishopPair = true;
        bool doPawnStructure = true;
        bool doKingSafety = true;
    };

    // ----------------------------
    // Search Options
    // ----------------------------
    struct SearchOptions {
        bool doQuiescense = true;
        bool doTranspositions = true;
        bool doBetaCutoffHistory = true;
        bool doInfo = true;

        int initialDepth = 9;
        int maximumDepth = 32;
        int maximumQuiescenseDepth = 16;

        bool doOpeningBook = false;
    };

    // ----------------------------
    // Master Configuration
    // ----------------------------
    struct Configuration {
        MoveGenOptions moveGen;
        EvalOptions eval;
        SearchOptions search;
    };

    // ----------------------------
    // Configuration Functions
    // ----------------------------
    int setOption(Rune::Game& game, const std::string& name, const std::string& value);
    void handleInput(Rune::Game& game, const std::string& input);

} // namespace Config