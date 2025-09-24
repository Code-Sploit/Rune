#pragma once

#include <array>

#include <tables/constants.hpp>

namespace Search::Tables {
    class Manager {
        private:
            // --- Beta cutoff history ---
            std::array<std::array<std::array<int, 64>, 64>, 2> betaCutoffHistory {};
        
        public:
            void addBetaCutoff(Move move, int depth, int turn);
            void updateBetaCutoffHistory();

            int getBetaCutoffEntry(int turn, int from, int to);
    };
}