#include <storage/repetition.hpp>
#include <tables/zobrist.hpp>

#include <core/attack.hpp>
#include <core/board.hpp>

#include <tables/magic.hpp>

#include <iostream>
#include <string>
#include <cstring>
#include <cctype>
#include <algorithm>

namespace Rune {
    class Game;
}

namespace Board {
    std::string generateFen(Rune::Game& game)
    {
        std::string fen_string;

        // 1. Generate piece placement
        for (int rank = 7; rank >= 0; rank--) {
            int empty_count = 0;

            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                Piece piece = game.boardGhost[square];

                if (Helpers::get_type(piece) == EMPTY) {
                    empty_count++;
                } else {
                    if (empty_count > 0) {
                        fen_string += ('0' + empty_count);
                        empty_count = 0;
                    }

                    char piece_char;
                    switch (Helpers::get_type(piece)) {
                        case PAWN:   piece_char = (Helpers::get_color(piece) == WHITE) ? 'P' : 'p'; break;
                        case KNIGHT: piece_char = (Helpers::get_color(piece) == WHITE) ? 'N' : 'n'; break;
                        case BISHOP: piece_char = (Helpers::get_color(piece) == WHITE) ? 'B' : 'b'; break;
                        case ROOK:   piece_char = (Helpers::get_color(piece) == WHITE) ? 'R' : 'r'; break;
                        case QUEEN:  piece_char = (Helpers::get_color(piece) == WHITE) ? 'Q' : 'q'; break;
                        case KING:   piece_char = (Helpers::get_color(piece) == WHITE) ? 'K' : 'k'; break;
                        default:     piece_char = ' '; break;
                    }
                    fen_string += piece_char;
                }
            }

            if (empty_count > 0) fen_string += ('0' + empty_count);
            if (rank > 0) fen_string += '/';
        }

        // 2. Add turn
        fen_string += (game.turn == WHITE) ? 'w' : 'b';
        fen_string += ' ';

        // 3. Add castling rights
        if (game.castlingRights == 0) fen_string += '-';
        else {
            if (game.castlingRights & 0x1) fen_string += 'K';
            if (game.castlingRights & 0x2) fen_string += 'Q';
            if (game.castlingRights & 0x4) fen_string += 'k';
            if (game.castlingRights & 0x8) fen_string += 'q';
        }
        fen_string += ' ';

        // 4. Add en passant, halfmove clock, fullmove number
        fen_string += "- 0 1";

        return fen_string;
    }

    void loadFen(Rune::Game& game, std::string fenString)
    {
        // Clear board
        std::fill(&game.board[0][0], &game.board[0][0] + 2*7*64, 0);

        const char* ptr = fenString.c_str();
        int square = 56; // A8

        // 1. Parse piece placement
        while (*ptr && *ptr != ' ')
        {
            char c = *ptr;

            if (isdigit(c))
            {
                int empty = c - '0';
                for (int i = 0; i < empty; i++)
                {
                    setSquare(game, square, Helpers::make_piece(EMPTY, EMPTY));
                    square++;
                }
            }
            else if (c == '/')
            {
                square -= 16; // Move down one rank
            }
            else
            {
                int color = isupper(c) ? WHITE : BLACK;
                int type;

                switch (tolower(c))
                {
                    case 'p': type = PAWN;   break;
                    case 'n': type = KNIGHT; break;
                    case 'b': type = BISHOP; break;
                    case 'r': type = ROOK;   break;
                    case 'q': type = QUEEN;  break;
                    case 'k': type = KING;   break;
                    default:  type = EMPTY;  break;
                }

                setSquare(game, square, Helpers::make_piece(type, color));
                square++;
            }

            ptr++;
        }

        // Skip space
        if (*ptr == ' ') ptr++;

        // 2. Side to move
        game.turn = (*ptr == 'w') ? WHITE : BLACK;

        while (*ptr && *ptr != ' ') ptr++;
        if (*ptr == ' ') ptr++;

        // 3. Castling rights
        game.castlingRights = 0;

        if (*ptr == '-')
        {
            ptr++;
        }
        else
        {
            while (*ptr && *ptr != ' ')
            {
                switch (*ptr)
                {
                    case 'K': game.castlingRights |= 0x1; break;
                    case 'Q': game.castlingRights |= 0x2; break;
                    case 'k': game.castlingRights |= 0x4; break;
                    case 'q': game.castlingRights |= 0x8; break;
                }
                ptr++;
            }
        }

        // (Optional: parse en passant, halfmove, fullmove here)

        // Recalculate tables only on *first* load

        game.attackWorker.generateTable(game, WHITE);
        game.attackWorker.generateTable(game, BLACK);

        Zobrist::updateBoard(game);
        
        game.isFirstLoad = 0;
    }

    std::string getCheckers(Rune::Game& game)
    {
        std::string buffer;  // single string to accumulate positions

        int us   = game.turn;
        int them = us ^ 1;

        // Find our king
        Bitboard king_bb = game.board[us][KING];

        if (!king_bb) return ""; // no king (illegal position)

        int king_sq = __builtin_ctzll(king_bb);

        // Current occupancy of all pieces
        Bitboard occupancy = game.occupancy[BOTH];

        // Bitboard of checking pieces
        Bitboard checkers_bb = 0ULL;

        for (int type = PAWN; type <= KING; type++)
        {
            Bitboard pieces = game.board[them][type];

            while (pieces)
            {
                int square = __builtin_ctzll(pieces);
                pieces &= pieces - 1;

                Bitboard attacks = 0ULL;

                switch (type) {
                    case PAWN:
                        attacks = game.attackWorker.preComputed.getPawnAttacks(them, square);
                        break;
                    case KNIGHT:
                        attacks = game.attackWorker.preComputed.getKnightAttacks(square);
                        break;
                    case BISHOP:
                        attacks = Magic::getBishopAttacks(square, occupancy);
                        break;
                    case ROOK:
                        attacks = Magic::getRookAttacks(square, occupancy);
                        break;
                    case QUEEN:
                        attacks = Magic::getQueenAttacks(square, occupancy);
                        break;
                    case KING:
                        attacks = game.attackWorker.preComputed.getKingAttacks(square);
                        break;
                }

                if (attacks & (1ULL << king_sq))
                {
                    checkers_bb |= (1ULL << square);
                }
            }
        }

        // Convert checkers bitboard to space-separated string
        while (checkers_bb)
        {
            int square = __builtin_ctzll(checkers_bb);

            checkers_bb &= checkers_bb - 1;

            char file = 'a' + (square % 8);
            char rank = '1' + (square / 8);

            buffer += file;
            buffer += rank;
            buffer += ' ';
        }

        if (!buffer.empty() && buffer.back() == ' ')
            buffer.pop_back(); // remove trailing space

        return buffer;
    }

    void print(Rune::Game& game)
    {
        const char piece_chars[7] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K'};

        std::cout << "\n";

        for (int rank = 7; rank >= 0; rank--)
        {
            std::cout << " +---+---+---+---+---+---+---+---+\n ";

            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;

                Piece p = game.boardGhost[square];

                if (p == 0)
                {
                    std::cout << "|   ";
                }
                else
                {
                    int type  = p & 0x07;
                    int color = (p & 0x08) >> 3;

                    char c = piece_chars[type];

                    if (color == 1) c = std::tolower(c);

                    std::cout << "| " << c << " ";
                }
            }

            std::cout << "| " << (rank + 1) << "  \n";
        }

        std::cout << " +---+---+---+---+---+---+---+---+\n ";
        std::cout << "  a   b   c   d   e   f   g   h \n\n";

        std::cout << "FEN: " << generateFen(game) << "\n";
        std::cout << "Zobrist Key: " << Zobrist::hashToString(game.zobristKey) << "\n";
        std::cout << "Checkers: " << getCheckers(game) << "\n\n";
    }

    void makeMove(Rune::Game& game, Move move, int callType)
    {
        const int from = Helpers::get_from(move);
        const int to   = Helpers::get_to(move);

        const Piece piece = game.boardGhost[from];
        const int color   = piece >> 3; // faster than helper call

        const bool ep = Helpers::is_enpassant(move);
        const int epCaptureSq = (color == WHITE) ? to - 8 : to + 8;
        const Piece captured = game.boardGhost[ep ? epCaptureSq : to];

        // Save state
        Rune::State* s = &game.history[game.historyCount++];

        s->castlingRights   = game.castlingRights;
        s->enpassantSquare  = game.enpassantSquare;
        s->capturedPiece    = captured;
        s->move             = move;
        s->turn             = game.turn;
        s->fiftyMoveCounter = game.repetitionTable.fiftyMoveCounter;

        if (callType == MAKE_MOVE_FULL) s->zobristKey = game.zobristKey;

        // Zobrist update
        if (callType == MAKE_MOVE_FULL)
            Zobrist::updateMove(game, move, *s);

        // Apply move
        setSquare(game, from, EMPTY);
        if (Helpers::is_promo(move))
            setSquare(game, to, Helpers::make_piece(Helpers::get_promo(move), color));
        else
            setSquare(game, to, piece);

        if (ep) setSquare(game, epCaptureSq, EMPTY);

        // Castling via lookup table
        if (Helpers::is_castle(move)) {
            auto cd = game.attackWorker.preComputed.castlingRookMoves[from][to];
            Piece rook = game.boardGhost[cd.rookFrom];
            setSquare(game, cd.rookFrom, EMPTY);
            setSquare(game, cd.rookTo, rook);
            game.hasCastled[color] = true;
        }

        // Update castling rights and en passant
        game.castlingRights &= game.attackWorker.preComputed.castling[from][to];
        game.enpassantSquare = (Helpers::is_double_push(move) ? (color == WHITE ? to - 8 : to + 8) : -1);

        // Incremental attack update
        game.attackWorker.update(game, move);

        // Switch turn
        game.turn ^= 1;
        game.ply++;

        // Repetition handling
        if (callType == MAKE_MOVE_FULL)
            game.repetitionTable.push(game.zobristKey);

        // Fifty-move rule
        if ((piece & 7) == PAWN || captured != EMPTY)
            game.repetitionTable.fiftyMoveCounter = 0;
        else
            game.repetitionTable.fiftyMoveCounter++;
    }

    void unmakeMove(Rune::Game& game, int callType)
    {
        Rune::State* s = &game.history[--game.historyCount];
        Move move = s->move;

        const int from = Helpers::get_from(move);
        const int to   = Helpers::get_to(move);
        Piece piece    = game.boardGhost[to];
        int color      = piece >> 3;

        // Undo turn flip first
        game.turn = s->turn;

        // Move back
        setSquare(game, to, EMPTY);
        if (Helpers::is_promo(move))
            setSquare(game, from, Helpers::make_piece(PAWN, color));
        else
            setSquare(game, from, piece);

        // Restore captured piece
        if (Helpers::is_enpassant(move)) {
            const int epCaptureSq = (color == WHITE) ? to - 8 : to + 8;
            setSquare(game, epCaptureSq, s->capturedPiece);
        } else if (s->capturedPiece != EMPTY) {
            setSquare(game, to, s->capturedPiece);
        }

        // Undo castling via lookup table
        if (Helpers::is_castle(move)) {
            auto cd = game.attackWorker.preComputed.castlingRookMoves[from][to];
            Piece rook = game.boardGhost[cd.rookTo];
            setSquare(game, cd.rookTo, EMPTY);
            setSquare(game, cd.rookFrom, rook);
            game.hasCastled[color] = false;
        }

        // Restore castling rights, en passant, zobrist
        game.castlingRights  = s->castlingRights;
        game.enpassantSquare = s->enpassantSquare;
        if (callType == MAKE_MOVE_FULL)
            game.zobristKey = s->zobristKey;

        // Restore incremental attack map
        game.attackWorker.restore(game, *s);

        // Repetition handling
        if (callType == MAKE_MOVE_FULL)
            game.repetitionTable.pop();
        game.repetitionTable.fiftyMoveCounter = s->fiftyMoveCounter;

        game.ply--;
    }

    void makeNullMove(Rune::Game& game)
    {
        // Save state
        Rune::State *s = &game.history[game.historyCount++];

        s->castlingRights  = game.castlingRights;
        s->enpassantSquare = game.enpassantSquare;
        s->zobristKey      = game.zobristKey;
        s->turn            = game.turn;

        // Update en passant square
        game.enpassantSquare = -1;

        // Switch turn
        game.turn ^= 1;

        // Zobrist update
        Zobrist::updateMove(game, 0, *s);
    }

    void unmakeNullMove(Rune::Game& game)
    {
        if (game.historyCount <= 0) {
            fprintf(stderr, "Error: unmake_move with empty history!\n");
            exit(EXIT_FAILURE);
        }

        Rune::State *s = &game.history[--game.historyCount];

        // Undo turn flip first (since make_move flips at the end)
        game.turn = s->turn;

        game.enpassantSquare = s->enpassantSquare;
        game.zobristKey = s->zobristKey;
    }

    bool isOnRank(int square, int rank)
    {
        return ((square / 8) == rank);
    }

    bool isOnFile(int square, int file)
    {
        return ((square % 8) == file);
    }

    std::string squareToName(int square)
    {
        if (square < 0 || square >= 64)
        {
            return "--";
        }

        int file = square % 8;
        int rank = square / 8;

        std::string name;

        name += static_cast<char>('a' + file);
        name += static_cast<char>('1' + rank);

        return name;
    }

    std::string moveToString(Move move)
    {
        // from and to squares
        std::string fromStr = squareToName(Helpers::get_from(move));
        std::string toStr   = squareToName(Helpers::get_to(move));

        std::string moveStr = fromStr + toStr;

        // promotion handling
        if (Helpers::get_promo(move) != 0)
        {
            Piece promoPiece = Helpers::get_promo(move);

            switch (Helpers::get_type(promoPiece))
            {
                case KNIGHT: moveStr += 'n'; break;
                case BISHOP: moveStr += 'b'; break;
                case ROOK:   moveStr += 'r'; break;
                case QUEEN:  moveStr += 'q'; break;
                default: break;
            }
        }

        return moveStr;
    }

    Move parseMove(Rune::Game& game, const std::string& moveStr)
    {
        if (moveStr.size() < 4)
            return Move{}; // null move

        int fromFile = moveStr[0] - 'a';
        int fromRank = moveStr[1] - '1';
        int toFile   = moveStr[2] - 'a';
        int toRank   = moveStr[3] - '1';

        int fromSq = fromRank * 8 + fromFile;
        int toSq   = toRank * 8 + toFile;

        // --- Promotion ---
        Piece promoFlag = EMPTY;

        bool isPromo = false;

        if (moveStr.size() >= 5)
        {
            isPromo = true;

            switch (moveStr[4])
            {
                case 'b': promoFlag = Helpers::make_piece(BISHOP, game.turn); break;
                case 'n': promoFlag = Helpers::make_piece(KNIGHT, game.turn); break;
                case 'r': promoFlag = Helpers::make_piece(ROOK,   game.turn); break;
                case 'q': promoFlag = Helpers::make_piece(QUEEN,  game.turn); break;
                default:  promoFlag = EMPTY; break; // safety fallback
            }
        }

        // --- Capture ---
        bool capture = (Helpers::get_type(game.boardGhost[toSq]) != EMPTY);

        // --- En passant ---
        bool enpassant = false;

        if (toSq == game.enpassantSquare && Helpers::get_type(game.boardGhost[fromSq]) == PAWN)
        {
            enpassant = true;
            capture   = true; // en passant is still a capture
        }

        // --- Double pawn push ---
        bool doublePush = false;

        if (Helpers::get_type(game.boardGhost[fromSq]) == PAWN && std::abs(toSq - fromSq) == 16)
            doublePush = true;

        // --- Castling ---
        bool castle = false;

        if (Helpers::get_type(game.boardGhost[fromSq]) == KING && std::abs(toSq - fromSq) == 2)
            castle = true;

        return Helpers::move(fromSq, toSq, promoFlag, capture, isPromo, enpassant, 0, doublePush, castle);
    }

    inline void setSquare(Rune::Game& game, int square, Piece piece)
    {
        const uint64_t bit = 1ULL << square;

        Piece old_piece = game.boardGhost[square];
        int old_type = Helpers::get_type(old_piece);

        // Remove old piece
        if (old_type != EMPTY) {
            int old_color = Helpers::get_color(old_piece);
            game.board[old_color][old_type] &= ~bit;
            game.occupancy[old_color] &= ~bit;
        }

        // Place new piece
        int new_type = Helpers::get_type(piece);
        if (new_type != EMPTY) {
            int new_color = Helpers::get_color(piece);
            game.board[new_color][new_type] |= bit;
            game.occupancy[new_color] |= bit;
        }

        game.boardGhost[square] = piece;

        // Combined occupancy
        game.occupancy[BOTH] = game.occupancy[WHITE] | game.occupancy[BLACK];
    }

    bool hasCastlingRights(Rune::Game& game, int side)
    {
        static const int castlingMasks[2] = {
            WHITE_KINGSIDE | WHITE_QUEENSIDE,
            BLACK_KINGSIDE | BLACK_QUEENSIDE
        };

        return (game.castlingRights & castlingMasks[side]) != 0;
    }

    bool hasCastlingRightsSide(Rune::Game& game, int side)
    {
        return (game.castlingRights & side) != 0;
    }

    bool isSameLine(int from, int to, int offset)
    {
        int from_rank = from / 8, from_file = from % 8;
        int to_rank = to / 8, to_file = to % 8;

        switch (offset) {
            case  1: case -1:  // horizontal
                return from_rank == to_rank;

            case  8: case -8:  // vertical
                return from_file == to_file;

            case  9: case -9:  // diagonal (↘ ↖)
                return (to_file - from_file) == (to_rank - from_rank);

            case  7: case -7:  // diagonal (↙ ↗)
                return (to_file - from_file) == -(to_rank - from_rank);

            default:
                return false;
        }
    }

    int findKing(Rune::Game& game, int color)
    {
        Bitboard king = game.board[color][KING];

        int kingSquare = -1;

        if (king)
        {
            kingSquare = __builtin_ctzll(king);
        }

        return kingSquare;
    }

    bool isKingInCheck(Rune::Game& game, int color)
    {
        int kingSquare = findKing(game, color);

        return (kingSquare < 0) ? false : (game.attackWorker.isSquareAttackedBy(kingSquare, !color));
    }

    bool moveGivesCheck(Rune::Game& game, Move move)
    {
        makeMove(game, move, MAKE_MOVE_LIGHT);

        bool check = isKingInCheck(game, game.turn);

        unmakeMove(game, MAKE_MOVE_LIGHT);

        return check;
    }

    Bitboard getSlidingPiecesBitboard(Rune::Game& game, int color)
    {
        return game.board[color][BISHOP] | game.board[color][ROOK] | game.board[color][QUEEN];
    }

    bool hasNonPawnMaterial(Rune::Game& game, int color)
    {
        return (game.board[color][KNIGHT] != 0 || game.board[color][BISHOP] != 0 ||
                game.board[color][ROOK] != 0 || game.board[color][QUEEN] != 0);
    }

    void skipTurn(Rune::Game& game)
    {
        game.turn ^= 1;

        Zobrist::updateBoard(game);
    }

    void undoSkipTurn(Rune::Game& game)
    {
        game.turn ^= 1;

        Zobrist::updateBoard(game);
    }

    bool hasFullMaterial(Rune::Game& game, int color)
    {
        Bitboard occupancy = game.occupancy[color];

        return (__builtin_popcountll(occupancy) == 20);
    }

    bool pawnChainsLocked(Rune::Game& game)
    {
        return (countPieces(game, PAWN) > 11);
    }

    int countPieces(Rune::Game& game, PieceType type)
    {
        Bitboard occupancy = game.board[WHITE][type] | game.board[BLACK][type];

        return (__builtin_popcountll(occupancy));
    }

    int hasPiece(Rune::Game& game, PieceType type, PieceColor color)
    {
        return (game.board[color][type] != 0);
    }

    int totalMaterial(Rune::Game &game)
    {
        int sum = 0;
        for (int color = 0; color < 2; ++color)
        {
            sum += 9 * __builtin_popcountll(game.board[color][QUEEN]);
            sum += 5 * __builtin_popcountll(game.board[color][ROOK]);
            sum += 3 * __builtin_popcountll(game.board[color][BISHOP]);
            sum += 3 * __builtin_popcountll(game.board[color][KNIGHT]);
            sum += 1 * __builtin_popcountll(game.board[color][PAWN]);
        }
        return sum;
    }

    bool isFileOpen(Rune::Game& game, int file)
    {
        Bitboard pawns = game.board[WHITE][PAWN] | game.board[BLACK][PAWN];
        Bitboard fileMask = FILE_MASKS[file];

        return !(fileMask & pawns);
    }

    bool isFileSemiOpen(Rune::Game& game, int file, int color)
    {
        Bitboard fileMask = FILE_MASKS[file];
        Bitboard friendlyPawns = game.board[color][PAWN];
        Bitboard enemyPawns = game.board[!color][PAWN];

        return !(fileMask & friendlyPawns) && (fileMask & enemyPawns);
    }

    int getPhase(Rune::Game& game)
    {
        // Piece indices: 0=P,1=N,2=B,3=R,4=Q
        const int piecePhase[5] = {0, 1, 1, 2, 4};
        int totalPhase = 0;

        for (int color = WHITE; color <= BLACK; color++) {
            for (int piece = KNIGHT; piece <= QUEEN; piece++) {
                int count = __builtin_popcountll(game.board[color][piece]);
                totalPhase += count * piecePhase[piece - 1];
            }
        }

        // Maximum phase in starting position = 16 (N/B/R/Q only)
        const int maxPhase = 16; 
        // Clamp to 0–maxPhase
        if (totalPhase > maxPhase) totalPhase = maxPhase;
        if (totalPhase < 0) totalPhase = 0;

        return totalPhase;
    }
}