// ==================== Standard Library ====================
#include <string>

// ==================== Core & Utilities ====================
#include <core/rune.hpp>
#include <core/attack.hpp>
#include <tables/helpers.hpp>

namespace Rune {
    class Game; // Forward declaration
}

namespace Board {
    // --------------------------------------------------------
    // FEN & Board helpers
    // --------------------------------------------------------
    std::string generateFen(Rune::Game& game);
    void        loadFen(Rune::Game& game, const std::string fenString);

    std::string getCheckers(Rune::Game& game);
    void        print(Rune::Game& game);

    bool isOnRank(int square, int rank);
    bool isOnFile(int square, int file);

    std::string squareToName(int square);
    std::string moveToString(Move move);
    Move        parseMove(Rune::Game& game, const std::string& moveStr);

    inline void setSquare(Rune::Game& game, int square, Piece piece);

    // --------------------------------------------------------
    // Move legality & castling
    // --------------------------------------------------------
    void makeMove(Rune::Game& game, Move move, int callType);
    void unmakeMove(Rune::Game& game, int callType);

    void makeNullMove(Rune::Game& game);
    void unmakeNullMove(Rune::Game& game);

    bool hasCastlingRights(Rune::Game& game, int side);
    bool hasCastlingRightsSide(Rune::Game& game, int side);

    bool isSameLine(int from, int to, int offset);
    int  findKing(Rune::Game& game, int color);
    bool isKingInCheck(Rune::Game& game, int color);
    //bool moveGivesCheck(Rune::Game& game, Move move);

    //Bitboard getSlidingPiecesBitboard(Rune::Game& game, int color);

    bool hasNonPawnMaterial(Rune::Game& game, int color);

    //void skipTurn(Rune::Game& game);
    //void undoSkipTurn(Rune::Game& game);

    bool hasFullMaterial(Rune::Game& game, int color);
    bool pawnChainsLocked(Rune::Game& game);

    int countPieces(Rune::Game& game, PieceType type);
    int hasPiece(Rune::Game& game, PieceType type, PieceColor color);
    int totalMaterial(Rune::Game& game);

    bool isFileOpen(Rune::Game& game, int file);
    bool isFileSemiOpen(Rune::Game& game, int file, int color);

    int getPhase(Rune::Game& game);
} // namespace Board