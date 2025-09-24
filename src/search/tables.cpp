#include <search/tables.hpp>
#include <search/config.hpp>

#include <tables/helpers.hpp>

namespace Search::Tables {
    void Manager::addBetaCutoff(Move move, int depth, int turn)
    {
        int from = Helpers::getFrom(move);
        int to   = Helpers::getTo(move);

        long long inc = (long long) depth * (long long) depth;

        betaCutoffHistory[turn][from][to] += (int) inc;

        if (betaCutoffHistory[turn][from][to] > Config::HISTORY_MAX)
            betaCutoffHistory[turn][from][to] = Config::HISTORY_MAX;
    }

    void Manager::updateBetaCutoffHistory()
    {
        for (int side = WHITE; side <= BLACK; side++)
        {
            for (int from = 0; from < 64; from++)
            {
                for (int to = 0; to < 64; to++)
                {
                    betaCutoffHistory[side][from][to] >>= 1;
                }
            }
        }
    }

    int Manager::getBetaCutoffEntry(int turn, int from, int to)
    {
        return betaCutoffHistory[turn][from][to];
    }
}