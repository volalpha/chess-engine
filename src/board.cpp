#include "board.hpp"
#include "zobrist.hpp"
#include "tt.hpp"
#include <iostream>
#include <sstream>
#include <cctype>

// Castling rights update mask for each of the 64 squares
static const std::uint8_t castlingRightsMask[64] = {
    13, 15, 15, 15, 12, 15, 15, 14, // Rank 1: a1(13), e1(12), h1(14)
    15, 15, 15, 15, 15, 15, 15, 15, // Rank 2
    15, 15, 15, 15, 15, 15, 15, 15, // Rank 3
    15, 15, 15, 15, 15, 15, 15, 15, // Rank 4
    15, 15, 15, 15, 15, 15, 15, 15, // Rank 5
    15, 15, 15, 15, 15, 15, 15, 15, // Rank 6
    15, 15, 15, 15, 15, 15, 15, 15, // Rank 7
     7, 15, 15, 15,  3, 15, 15, 11  // Rank 8: a8(7), e8(3), h8(11)
};

Board::Board()
    : pieces{},
      whiteOccupancy(0),
      blackOccupancy(0),
      occupancy(0),
      sideToMove(Color::White),
      castlingRights(ALL_CASTLE),
      enPassantSquare(-1),
      zobristKey(0),
      abortSearch(false),
      searchTimeLimitMs(0)
{
}

void Board::clearBoard()
{
    for (int i = 0; i < static_cast<int>(Piece::Count); ++i)
    {
        pieces[i] = 0ULL;
    }
    whiteOccupancy = 0ULL;
    blackOccupancy = 0ULL;
    occupancy = 0ULL;
    sideToMove = Color::White;
    castlingRights = 0;
    enPassantSquare = -1;
    zobristKey = 0;

    for (int i = 0; i < 64; ++i)
    {
        killerMoves[i][0] = 0;
        killerMoves[i][1] = 0;
    }
    for (int i = 0; i < 12; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            historyTable[i][j] = 0;
        }
    }
}

void Board::setupFen(const std::string& fen)
{
    clearBoard();

    std::stringstream ss(fen);
    std::string piecePlacement, activeColor, castling, enPassant;
    ss >> piecePlacement >> activeColor >> castling >> enPassant;

    // 1. Piece placement
    int rank = 7; // Rank 8 (index 7)
    int file = 0; // File 'a' (index 0)

    for (char c : piecePlacement)
    {
        if (c == '/')
        {
            rank--;
            file = 0;
        }
        else if (std::isdigit(c))
        {
            file += (c - '0');
        }
        else
        {
            int square = rank * 8 + file;
            switch (c)
            {
                case 'P': setPiece(Piece::WhitePawn, square); break;
                case 'N': setPiece(Piece::WhiteKnight, square); break;
                case 'B': setPiece(Piece::WhiteBishop, square); break;
                case 'R': setPiece(Piece::WhiteRook, square); break;
                case 'Q': setPiece(Piece::WhiteQueen, square); break;
                case 'K': setPiece(Piece::WhiteKing, square); break;

                case 'p': setPiece(Piece::BlackPawn, square); break;
                case 'n': setPiece(Piece::BlackKnight, square); break;
                case 'b': setPiece(Piece::BlackBishop, square); break;
                case 'r': setPiece(Piece::BlackRook, square); break;
                case 'q': setPiece(Piece::BlackQueen, square); break;
                case 'k': setPiece(Piece::BlackKing, square); break;
            }
            file++;
        }
    }

    // 2. Active color
    if (activeColor == "w")
    {
        sideToMove = Color::White;
    }
    else if (activeColor == "b")
    {
        sideToMove = Color::Black;
    }

    // 3. Castling rights
    castlingRights = 0;
    if (castling != "-")
    {
        for (char c : castling)
        {
            if (c == 'K') castlingRights |= WK_CASTLE;
            if (c == 'Q') castlingRights |= WQ_CASTLE;
            if (c == 'k') castlingRights |= BK_CASTLE;
            if (c == 'q') castlingRights |= BQ_CASTLE;
        }
    }

    // 4. En-passant target square
    if (enPassant != "-" && enPassant.length() >= 2)
    {
        int fileIdx = enPassant[0] - 'a';
        int rankIdx = enPassant[1] - '1';
        enPassantSquare = rankIdx * 8 + fileIdx;
    }

    // 5. Update occupancies
    updateOccupancies();

    zobristKey = Zobrist::computeFullKey(*this);
}

void Board::setStartingPosition()
{
    // Clear all piece bitboards
    for (int i = 0; i < static_cast<int>(Piece::Count); ++i)
    {
        pieces[i] = 0;
    }

    // White pawns: rank 2 -> squares 8-15
    for (int file = 0; file < 8; ++file)
    {
        setBit(pieces[static_cast<int>(Piece::WhitePawn)], 8 + file);
        setBit(pieces[static_cast<int>(Piece::BlackPawn)], 48 + file);
    }

    // White pieces
    setBit(pieces[static_cast<int>(Piece::WhiteRook)], 0);
    setBit(pieces[static_cast<int>(Piece::WhiteRook)], 7);
    setBit(pieces[static_cast<int>(Piece::WhiteKnight)], 1);
    setBit(pieces[static_cast<int>(Piece::WhiteKnight)], 6);
    setBit(pieces[static_cast<int>(Piece::WhiteBishop)], 2);
    setBit(pieces[static_cast<int>(Piece::WhiteBishop)], 5);
    setBit(pieces[static_cast<int>(Piece::WhiteQueen)], 3);
    setBit(pieces[static_cast<int>(Piece::WhiteKing)], 4);

    // Black pieces
    setBit(pieces[static_cast<int>(Piece::BlackRook)], 56);
    setBit(pieces[static_cast<int>(Piece::BlackRook)], 63);
    setBit(pieces[static_cast<int>(Piece::BlackKnight)], 57);
    setBit(pieces[static_cast<int>(Piece::BlackKnight)], 62);
    setBit(pieces[static_cast<int>(Piece::BlackBishop)], 58);
    setBit(pieces[static_cast<int>(Piece::BlackBishop)], 61);
    setBit(pieces[static_cast<int>(Piece::BlackQueen)], 59);
    setBit(pieces[static_cast<int>(Piece::BlackKing)], 60);

    updateOccupancies();

    sideToMove = Color::White;
    castlingRights = ALL_CASTLE;
    enPassantSquare = -1;

    zobristKey = Zobrist::computeFullKey(*this);
}

void Board::updateOccupancies()
{
    whiteOccupancy =
        pieces[static_cast<int>(Piece::WhitePawn)] |
        pieces[static_cast<int>(Piece::WhiteKnight)] |
        pieces[static_cast<int>(Piece::WhiteBishop)] |
        pieces[static_cast<int>(Piece::WhiteRook)] |
        pieces[static_cast<int>(Piece::WhiteQueen)] |
        pieces[static_cast<int>(Piece::WhiteKing)];

    blackOccupancy =
        pieces[static_cast<int>(Piece::BlackPawn)] |
        pieces[static_cast<int>(Piece::BlackKnight)] |
        pieces[static_cast<int>(Piece::BlackBishop)] |
        pieces[static_cast<int>(Piece::BlackRook)] |
        pieces[static_cast<int>(Piece::BlackQueen)] |
        pieces[static_cast<int>(Piece::BlackKing)];

    occupancy = whiteOccupancy | blackOccupancy;
}

Piece Board::getPieceAt(int square) const
{
    for (int i = 0; i < static_cast<int>(Piece::Count); ++i)
    {
        if (getBit(pieces[i], square))
            return static_cast<Piece>(i);
    }
    return Piece::Count;
}

Bitboard Board::getPieceBitboard(Piece piece) const { return pieces[static_cast<int>(piece)]; }
Bitboard Board::getOccupancy() const { return occupancy; }
Bitboard Board::getWhiteOccupancy() const { return whiteOccupancy; }
Bitboard Board::getBlackOccupancy() const { return blackOccupancy; }
Color Board::getSideToMove() const { return sideToMove; }
std::uint8_t Board::getCastlingRights() const { return castlingRights; }
int Board::getEnPassantSquare() const { return enPassantSquare; }
std::uint64_t Board::getZobristKey() const { return zobristKey; }

void Board::setPiece(Piece piece, int square)
{
    setBit(pieces[static_cast<int>(piece)], square);
    updateOccupancies();
}

void Board::removePiece(Piece piece, int square)
{
    clearBit(pieces[static_cast<int>(piece)], square);
    updateOccupancies();
}

void Board::setSideToMove(Color side) { sideToMove = side; }
void Board::setCastlingRights(std::uint8_t rights) { castlingRights = rights; }
void Board::setEnPassantSquare(int square) { enPassantSquare = square; }

bool Board::operator==(const Board& other) const
{
    if (sideToMove != other.sideToMove) return false;
    if (castlingRights != other.castlingRights) return false;
    if (enPassantSquare != other.enPassantSquare) return false;
    if (whiteOccupancy != other.whiteOccupancy) return false;
    if (blackOccupancy != other.blackOccupancy) return false;
    if (occupancy != other.occupancy) return false;

    for (int i = 0; i < static_cast<int>(Piece::Count); ++i)
    {
        if (pieces[i] != other.pieces[i]) return false;
    }
    return true;
}

UndoState Board::makeMove(Move move)
{
    UndoState undo;
    undo.capturedPiece = Piece::Count;
    undo.enPassantSquare = enPassantSquare;
    undo.castlingRights = castlingRights;
    undo.zobristKey = zobristKey;

    // Remove old en-passant key
    if (enPassantSquare != -1)
    {
        zobristKey ^= Zobrist::enPassantKeys[enPassantSquare % 8];
    }

    // Remove old castling rights key
    zobristKey ^= Zobrist::castlingKeys[castlingRights];

    int from = getMoveFrom(move);
    int to = getMoveTo(move);
    MoveFlag flags = getMoveFlags(move);
    Piece movingPiece = getPieceAt(from);

    // 1. Normal capture (non-en-passant)
    if (isMoveCapture(move) && flags != FlagEnPassant)
    {
        undo.capturedPiece = getPieceAt(to);
        if (undo.capturedPiece != Piece::Count)
        {
            clearBit(pieces[static_cast<int>(undo.capturedPiece)], to);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(undo.capturedPiece)][to];
        }
    }

    // 2. Move the moving piece from source square
    clearBit(pieces[static_cast<int>(movingPiece)], from);
    zobristKey ^= Zobrist::pieceKeys[static_cast<int>(movingPiece)][from];

    // 3. Place piece on destination square (or promoted piece if promotion)
    if (isMovePromotion(move))
    {
        Piece promotedPiece;
        if (sideToMove == Color::White)
        {
            switch (flags)
            {
                case FlagKnightProm: case FlagKnightPromCap: promotedPiece = Piece::WhiteKnight; break;
                case FlagBishopProm: case FlagBishopPromCap: promotedPiece = Piece::WhiteBishop; break;
                case FlagRookProm:   case FlagRookPromCap:   promotedPiece = Piece::WhiteRook; break;
                default:                                     promotedPiece = Piece::WhiteQueen; break;
            }
        }
        else
        {
            switch (flags)
            {
                case FlagKnightProm: case FlagKnightPromCap: promotedPiece = Piece::BlackKnight; break;
                case FlagBishopProm: case FlagBishopPromCap: promotedPiece = Piece::BlackBishop; break;
                case FlagRookProm:   case FlagRookPromCap:   promotedPiece = Piece::BlackRook; break;
                default:                                     promotedPiece = Piece::BlackQueen; break;
            }
        }
        setBit(pieces[static_cast<int>(promotedPiece)], to);
        zobristKey ^= Zobrist::pieceKeys[static_cast<int>(promotedPiece)][to];
    }
    else
    {
        setBit(pieces[static_cast<int>(movingPiece)], to);
        zobristKey ^= Zobrist::pieceKeys[static_cast<int>(movingPiece)][to];
    }

    // 4. Special move handling: Castling & En Passant
    if (flags == FlagKingCastle)
    {
        if (sideToMove == Color::White)
        {
            // White O-O: Rook moves h1 (7) -> f1 (5)
            clearBit(pieces[static_cast<int>(Piece::WhiteRook)], 7);
            setBit(pieces[static_cast<int>(Piece::WhiteRook)], 5);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::WhiteRook)][7];
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::WhiteRook)][5];
        }
        else
        {
            // Black O-O: Rook moves h8 (63) -> f8 (61)
            clearBit(pieces[static_cast<int>(Piece::BlackRook)], 63);
            setBit(pieces[static_cast<int>(Piece::BlackRook)], 61);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::BlackRook)][63];
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::BlackRook)][61];
        }
    }
    else if (flags == FlagQueenCastle)
    {
        if (sideToMove == Color::White)
        {
            // White O-O-O: Rook moves a1 (0) -> d1 (3)
            clearBit(pieces[static_cast<int>(Piece::WhiteRook)], 0);
            setBit(pieces[static_cast<int>(Piece::WhiteRook)], 3);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::WhiteRook)][0];
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::WhiteRook)][3];
        }
        else
        {
            // Black O-O-O: Rook moves a8 (56) -> d8 (59)
            clearBit(pieces[static_cast<int>(Piece::BlackRook)], 56);
            setBit(pieces[static_cast<int>(Piece::BlackRook)], 59);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::BlackRook)][56];
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::BlackRook)][59];
        }
    }
    else if (flags == FlagEnPassant)
    {
        if (sideToMove == Color::White)
        {
            // White capturing black pawn on to - 8
            int targetPawnSq = to - 8;
            undo.capturedPiece = Piece::BlackPawn;
            clearBit(pieces[static_cast<int>(Piece::BlackPawn)], targetPawnSq);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::BlackPawn)][targetPawnSq];
        }
        else
        {
            // Black capturing white pawn on to + 8
            int targetPawnSq = to + 8;
            undo.capturedPiece = Piece::WhitePawn;
            clearBit(pieces[static_cast<int>(Piece::WhitePawn)], targetPawnSq);
            zobristKey ^= Zobrist::pieceKeys[static_cast<int>(Piece::WhitePawn)][targetPawnSq];
        }
    }

    // 5. Update en-passant square
    enPassantSquare = -1;
    if (flags == FlagDoublePawnPush)
    {
        enPassantSquare = (sideToMove == Color::White) ? (from + 8) : (from - 8);
        zobristKey ^= Zobrist::enPassantKeys[enPassantSquare % 8];
    }

    // 6. Update castling rights
    castlingRights &= castlingRightsMask[from] & castlingRightsMask[to];
    zobristKey ^= Zobrist::castlingKeys[castlingRights];

    // 7. Toggle side to move & update occupancies
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    zobristKey ^= Zobrist::sideKey;
    updateOccupancies();

    return undo;
}

void Board::unmakeMove(Move move, const UndoState& undo)
{
    // O(1) state restore for Zobrist hashing
    zobristKey = undo.zobristKey;

    // 1. Toggle side to move back
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;

    // 2. Restore castling rights & en-passant square
    castlingRights = undo.castlingRights;
    enPassantSquare = undo.enPassantSquare;

    int from = getMoveFrom(move);
    int to = getMoveTo(move);
    MoveFlag flags = getMoveFlags(move);
    Piece movingPiece = getPieceAt(to);

    // 3. Revert promotions or normal piece placement
    if (isMovePromotion(move))
    {
        // Remove promoted piece from `to` square
        clearBit(pieces[static_cast<int>(movingPiece)], to);

        // Restore original Pawn on `from` square
        Piece pawnPiece = (sideToMove == Color::White) ? Piece::WhitePawn : Piece::BlackPawn;
        setBit(pieces[static_cast<int>(pawnPiece)], from);
    }
    else
    {
        // Move piece back from `to` to `from`
        clearBit(pieces[static_cast<int>(movingPiece)], to);
        setBit(pieces[static_cast<int>(movingPiece)], from);
    }

    // 4. Restore captures
    if (flags == FlagEnPassant)
    {
        if (sideToMove == Color::White)
        {
            int targetPawnSq = to - 8;
            setBit(pieces[static_cast<int>(Piece::BlackPawn)], targetPawnSq);
        }
        else
        {
            int targetPawnSq = to + 8;
            setBit(pieces[static_cast<int>(Piece::WhitePawn)], targetPawnSq);
        }
    }
    else if (isMoveCapture(move) && undo.capturedPiece != Piece::Count)
    {
        setBit(pieces[static_cast<int>(undo.capturedPiece)], to);
    }

    // 5. Revert Castling Rook moves
    if (flags == FlagKingCastle)
    {
        if (sideToMove == Color::White)
        {
            clearBit(pieces[static_cast<int>(Piece::WhiteRook)], 5);
            setBit(pieces[static_cast<int>(Piece::WhiteRook)], 7);
        }
        else
        {
            clearBit(pieces[static_cast<int>(Piece::BlackRook)], 61);
            setBit(pieces[static_cast<int>(Piece::BlackRook)], 63);
        }
    }
    else if (flags == FlagQueenCastle)
    {
        if (sideToMove == Color::White)
        {
            clearBit(pieces[static_cast<int>(Piece::WhiteRook)], 3);
            setBit(pieces[static_cast<int>(Piece::WhiteRook)], 0);
        }
        else
        {
            clearBit(pieces[static_cast<int>(Piece::BlackRook)], 59);
            setBit(pieces[static_cast<int>(Piece::BlackRook)], 56);
        }
    }

    // 6. Recalculate occupancies
    updateOccupancies();
}

// Attack Detection & Move Generation

bool Board::isSquareAttacked(int square, Color attackerColor) const
{
    // 1. Pawn Attacks
    if (attackerColor == Color::White)
    {
        if (getPawnAttacks(1, square) & pieces[static_cast<int>(Piece::WhitePawn)])
            return true;
    }
    else
    {
        if (getPawnAttacks(0, square) & pieces[static_cast<int>(Piece::BlackPawn)])
            return true;
    }

    // 2. Knight Attacks
    Bitboard attackerKnights = (attackerColor == Color::White) ? pieces[static_cast<int>(Piece::WhiteKnight)] : pieces[static_cast<int>(Piece::BlackKnight)];
    if (getKnightAttacks(square) & attackerKnights)
        return true;

    // 3. King Attacks
    Bitboard attackerKing = (attackerColor == Color::White) ? pieces[static_cast<int>(Piece::WhiteKing)] : pieces[static_cast<int>(Piece::BlackKing)];
    if (getKingAttacks(square) & attackerKing)
        return true;

    // 4. Bishop & Queen Attacks (Diagonal)
    Bitboard attackerBishopsQueens = (attackerColor == Color::White) ?
        (pieces[static_cast<int>(Piece::WhiteBishop)] | pieces[static_cast<int>(Piece::WhiteQueen)]) :
        (pieces[static_cast<int>(Piece::BlackBishop)] | pieces[static_cast<int>(Piece::BlackQueen)]);
    if (getBishopAttacks(square, occupancy) & attackerBishopsQueens)
        return true;

    // 5. Rook & Queen Attacks (Straight)
    Bitboard attackerRooksQueens = (attackerColor == Color::White) ?
        (pieces[static_cast<int>(Piece::WhiteRook)] | pieces[static_cast<int>(Piece::WhiteQueen)]) :
        (pieces[static_cast<int>(Piece::BlackRook)] | pieces[static_cast<int>(Piece::BlackQueen)]);
    if (getRookAttacks(square, occupancy) & attackerRooksQueens)
        return true;

    return false;
}

bool Board::isKingInCheck(Color kingColor) const
{
    Bitboard kingBb = (kingColor == Color::White) ? pieces[static_cast<int>(Piece::WhiteKing)] : pieces[static_cast<int>(Piece::BlackKing)];
    if (!kingBb)
        return false;

    int kingSq = __builtin_ctzll(kingBb);
    Color enemyColor = (kingColor == Color::White) ? Color::Black : Color::White;
    return isSquareAttacked(kingSq, enemyColor);
}

std::vector<Move> Board::generatePseudoLegalMoves() const
{
    std::vector<Move> moves;
    moves.reserve(64);

    Bitboard ownOccupancy = (sideToMove == Color::White) ? whiteOccupancy : blackOccupancy;
    Bitboard enemyOccupancy = (sideToMove == Color::White) ? blackOccupancy : whiteOccupancy;

    // 1. Pawn Moves
    int pawnEnumIdx = (sideToMove == Color::White) ? static_cast<int>(Piece::WhitePawn) : static_cast<int>(Piece::BlackPawn);
    Bitboard pawns = pieces[pawnEnumIdx];

    while (pawns)
    {
        int fromSq = __builtin_ctzll(pawns);
        clearBit(pawns, fromSq);

        int rank = fromSq / 8;

        if (sideToMove == Color::White)
        {
            // Single Push
            int toSq = fromSq + 8;
            if (!getBit(occupancy, toSq))
            {
                if (toSq >= 56) // Rank 8 Promotion
                {
                    moves.push_back(encodeMove(fromSq, toSq, FlagQueenProm));
                    moves.push_back(encodeMove(fromSq, toSq, FlagRookProm));
                    moves.push_back(encodeMove(fromSq, toSq, FlagBishopProm));
                    moves.push_back(encodeMove(fromSq, toSq, FlagKnightProm));
                }
                else
                {
                    moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));

                    // Double Push (Rank 2 -> Rank 4)
                    if (rank == 1)
                    {
                        int doublePushSq = fromSq + 16;
                        if (!getBit(occupancy, doublePushSq))
                        {
                            moves.push_back(encodeMove(fromSq, doublePushSq, FlagDoublePawnPush));
                        }
                    }
                }
            }

            // Pawn Captures
            Bitboard attacks = getPawnAttacks(0, fromSq) & enemyOccupancy;
            while (attacks)
            {
                int capSq = __builtin_ctzll(attacks);
                clearBit(attacks, capSq);

                if (capSq >= 56) // Promotion Capture
                {
                    moves.push_back(encodeMove(fromSq, capSq, FlagQueenPromCap));
                    moves.push_back(encodeMove(fromSq, capSq, FlagRookPromCap));
                    moves.push_back(encodeMove(fromSq, capSq, FlagBishopPromCap));
                    moves.push_back(encodeMove(fromSq, capSq, FlagKnightPromCap));
                }
                else
                {
                    moves.push_back(encodeMove(fromSq, capSq, FlagCapture));
                }
            }

            // En Passant
            if (enPassantSquare != -1)
            {
                Bitboard epAttacks = getPawnAttacks(0, fromSq) & (1ULL << enPassantSquare);
                if (epAttacks)
                {
                    moves.push_back(encodeMove(fromSq, enPassantSquare, FlagEnPassant));
                }
            }
        }
        else // Black Pawns
        {
            // Single Push
            int toSq = fromSq - 8;
            if (!getBit(occupancy, toSq))
            {
                if (toSq <= 7) // Rank 1 Promotion
                {
                    moves.push_back(encodeMove(fromSq, toSq, FlagQueenProm));
                    moves.push_back(encodeMove(fromSq, toSq, FlagRookProm));
                    moves.push_back(encodeMove(fromSq, toSq, FlagBishopProm));
                    moves.push_back(encodeMove(fromSq, toSq, FlagKnightProm));
                }
                else
                {
                    moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));

                    // Double Push (Rank 7 -> Rank 5)
                    if (rank == 6)
                    {
                        int doublePushSq = fromSq - 16;
                        if (!getBit(occupancy, doublePushSq))
                        {
                            moves.push_back(encodeMove(fromSq, doublePushSq, FlagDoublePawnPush));
                        }
                    }
                }
            }

            // Pawn Captures
            Bitboard attacks = getPawnAttacks(1, fromSq) & enemyOccupancy;
            while (attacks)
            {
                int capSq = __builtin_ctzll(attacks);
                clearBit(attacks, capSq);

                if (capSq <= 7) // Promotion Capture
                {
                    moves.push_back(encodeMove(fromSq, capSq, FlagQueenPromCap));
                    moves.push_back(encodeMove(fromSq, capSq, FlagRookPromCap));
                    moves.push_back(encodeMove(fromSq, capSq, FlagBishopPromCap));
                    moves.push_back(encodeMove(fromSq, capSq, FlagKnightPromCap));
                }
                else
                {
                    moves.push_back(encodeMove(fromSq, capSq, FlagCapture));
                }
            }

            // En Passant
            if (enPassantSquare != -1)
            {
                Bitboard epAttacks = getPawnAttacks(1, fromSq) & (1ULL << enPassantSquare);
                if (epAttacks)
                {
                    moves.push_back(encodeMove(fromSq, enPassantSquare, FlagEnPassant));
                }
            }
        }
    }

    // 2. Knight Moves
    int knightIdx = (sideToMove == Color::White) ? static_cast<int>(Piece::WhiteKnight) : static_cast<int>(Piece::BlackKnight);
    Bitboard knights = pieces[knightIdx];
    while (knights)
    {
        int fromSq = __builtin_ctzll(knights);
        clearBit(knights, fromSq);

        Bitboard attacks = getKnightAttacks(fromSq) & ~ownOccupancy;
        while (attacks)
        {
            int toSq = __builtin_ctzll(attacks);
            clearBit(attacks, toSq);

            if (getBit(enemyOccupancy, toSq))
                moves.push_back(encodeMove(fromSq, toSq, FlagCapture));
            else
                moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));
        }
    }

    // 3. Bishop Moves
    int bishopIdx = (sideToMove == Color::White) ? static_cast<int>(Piece::WhiteBishop) : static_cast<int>(Piece::BlackBishop);
    Bitboard bishops = pieces[bishopIdx];
    while (bishops)
    {
        int fromSq = __builtin_ctzll(bishops);
        clearBit(bishops, fromSq);

        Bitboard attacks = getBishopAttacks(fromSq, occupancy) & ~ownOccupancy;
        while (attacks)
        {
            int toSq = __builtin_ctzll(attacks);
            clearBit(attacks, toSq);

            if (getBit(enemyOccupancy, toSq))
                moves.push_back(encodeMove(fromSq, toSq, FlagCapture));
            else
                moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));
        }
    }

    // 4. Rook Moves
    int rookIdx = (sideToMove == Color::White) ? static_cast<int>(Piece::WhiteRook) : static_cast<int>(Piece::BlackRook);
    Bitboard rooks = pieces[rookIdx];
    while (rooks)
    {
        int fromSq = __builtin_ctzll(rooks);
        clearBit(rooks, fromSq);

        Bitboard attacks = getRookAttacks(fromSq, occupancy) & ~ownOccupancy;
        while (attacks)
        {
            int toSq = __builtin_ctzll(attacks);
            clearBit(attacks, toSq);

            if (getBit(enemyOccupancy, toSq))
                moves.push_back(encodeMove(fromSq, toSq, FlagCapture));
            else
                moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));
        }
    }

    // 5. Queen Moves
    int queenIdx = (sideToMove == Color::White) ? static_cast<int>(Piece::WhiteQueen) : static_cast<int>(Piece::BlackQueen);
    Bitboard queens = pieces[queenIdx];
    while (queens)
    {
        int fromSq = __builtin_ctzll(queens);
        clearBit(queens, fromSq);

        Bitboard attacks = getQueenAttacks(fromSq, occupancy) & ~ownOccupancy;
        while (attacks)
        {
            int toSq = __builtin_ctzll(attacks);
            clearBit(attacks, toSq);

            if (getBit(enemyOccupancy, toSq))
                moves.push_back(encodeMove(fromSq, toSq, FlagCapture));
            else
                moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));
        }
    }

    // 6. King Moves & Castling
    int kingIdx = (sideToMove == Color::White) ? static_cast<int>(Piece::WhiteKing) : static_cast<int>(Piece::BlackKing);
    Bitboard kingBb = pieces[kingIdx];
    if (kingBb)
    {
        int fromSq = __builtin_ctzll(kingBb);
        Bitboard attacks = getKingAttacks(fromSq) & ~ownOccupancy;
        while (attacks)
        {
            int toSq = __builtin_ctzll(attacks);
            clearBit(attacks, toSq);

            if (getBit(enemyOccupancy, toSq))
                moves.push_back(encodeMove(fromSq, toSq, FlagCapture));
            else
                moves.push_back(encodeMove(fromSq, toSq, FlagQuiet));
        }

        // Castling Rules:
        Color enemyColor = (sideToMove == Color::White) ? Color::Black : Color::White;

        if (sideToMove == Color::White)
        {
            // White Kingside O-O (e1-g1)
            if (castlingRights & WK_CASTLE)
            {
                // f1 (5) and g1 (6) empty
                if (!getBit(occupancy, 5) && !getBit(occupancy, 6))
                {
                    // e1 (4), f1 (5), g1 (6) not attacked
                    if (!isSquareAttacked(4, enemyColor) && !isSquareAttacked(5, enemyColor) && !isSquareAttacked(6, enemyColor))
                    {
                        moves.push_back(encodeMove(4, 6, FlagKingCastle));
                    }
                }
            }
            // White Queenside O-O-O (e1-c1)
            if (castlingRights & WQ_CASTLE)
            {
                // b1 (1), c1 (2), d1 (3) empty
                if (!getBit(occupancy, 1) && !getBit(occupancy, 2) && !getBit(occupancy, 3))
                {
                    // e1 (4), d1 (3), c1 (2) not attacked
                    if (!isSquareAttacked(4, enemyColor) && !isSquareAttacked(3, enemyColor) && !isSquareAttacked(2, enemyColor))
                    {
                        moves.push_back(encodeMove(4, 2, FlagQueenCastle));
                    }
                }
            }
        }
        else // Black Castling
        {
            // Black Kingside O-O (e8-g8)
            if (castlingRights & BK_CASTLE)
            {
                // f8 (61) and g8 (62) empty
                if (!getBit(occupancy, 61) && !getBit(occupancy, 62))
                {
                    // e8 (60), f8 (61), g8 (62) not attacked
                    if (!isSquareAttacked(60, enemyColor) && !isSquareAttacked(61, enemyColor) && !isSquareAttacked(62, enemyColor))
                    {
                        moves.push_back(encodeMove(60, 62, FlagKingCastle));
                    }
                }
            }
            // Black Queenside O-O-O (e8-c8)
            if (castlingRights & BQ_CASTLE)
            {
                // b8 (57), c8 (58), d8 (59) empty
                if (!getBit(occupancy, 57) && !getBit(occupancy, 58) && !getBit(occupancy, 59))
                {
                    // e8 (60), d8 (59), c8 (58) not attacked
                    if (!isSquareAttacked(60, enemyColor) && !isSquareAttacked(59, enemyColor) && !isSquareAttacked(58, enemyColor))
                    {
                        moves.push_back(encodeMove(60, 58, FlagQueenCastle));
                    }
                }
            }
        }
    }

    return moves;
}

std::vector<Move> Board::generateLegalMoves()
{
    std::vector<Move> pseudoMoves = generatePseudoLegalMoves();
    std::vector<Move> legalMoves;
    legalMoves.reserve(pseudoMoves.size());

    Color movingSide = sideToMove;

    for (Move move : pseudoMoves)
    {
        UndoState undo = makeMove(move);

        if (!isKingInCheck(movingSide))
        {
            legalMoves.push_back(move);
        }

        unmakeMove(move, undo);
    }

    return legalMoves;
}

// ============================================================================
// STATIC EVALUATION & PIECE-SQUARE TABLES (PST)
// ============================================================================

static const int pawnPST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int knightPST[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

static const int bishopPST[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

static const int rookPST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

static const int queenPST[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

static const int kingPST[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};

int Board::evaluate() const
{
    int whiteScore = 0;
    int blackScore = 0;

    constexpr int PAWN_VAL   = 100;
    constexpr int KNIGHT_VAL = 320;
    constexpr int BISHOP_VAL = 330;
    constexpr int ROOK_VAL   = 500;
    constexpr int QUEEN_VAL  = 900;
    constexpr int KING_VAL   = 20000;

    // 1. Pawns
    Bitboard wPawns = pieces[static_cast<int>(Piece::WhitePawn)];
    while (wPawns) {
        int sq = __builtin_ctzll(wPawns);
        clearBit(wPawns, sq);
        whiteScore += PAWN_VAL + pawnPST[sq];
    }
    Bitboard bPawns = pieces[static_cast<int>(Piece::BlackPawn)];
    while (bPawns) {
        int sq = __builtin_ctzll(bPawns);
        clearBit(bPawns, sq);
        blackScore += PAWN_VAL + pawnPST[sq ^ 56];
    }

    // 2. Knights
    Bitboard wKnights = pieces[static_cast<int>(Piece::WhiteKnight)];
    while (wKnights) {
        int sq = __builtin_ctzll(wKnights);
        clearBit(wKnights, sq);
        whiteScore += KNIGHT_VAL + knightPST[sq];
    }
    Bitboard bKnights = pieces[static_cast<int>(Piece::BlackKnight)];
    while (bKnights) {
        int sq = __builtin_ctzll(bKnights);
        clearBit(bKnights, sq);
        blackScore += KNIGHT_VAL + knightPST[sq ^ 56];
    }

    // 3. Bishops
    Bitboard wBishops = pieces[static_cast<int>(Piece::WhiteBishop)];
    while (wBishops) {
        int sq = __builtin_ctzll(wBishops);
        clearBit(wBishops, sq);
        whiteScore += BISHOP_VAL + bishopPST[sq];
    }
    Bitboard bBishops = pieces[static_cast<int>(Piece::BlackBishop)];
    while (bBishops) {
        int sq = __builtin_ctzll(bBishops);
        clearBit(bBishops, sq);
        blackScore += BISHOP_VAL + bishopPST[sq ^ 56];
    }

    // 4. Rooks
    Bitboard wRooks = pieces[static_cast<int>(Piece::WhiteRook)];
    while (wRooks) {
        int sq = __builtin_ctzll(wRooks);
        clearBit(wRooks, sq);
        whiteScore += ROOK_VAL + rookPST[sq];
    }
    Bitboard bRooks = pieces[static_cast<int>(Piece::BlackRook)];
    while (bRooks) {
        int sq = __builtin_ctzll(bRooks);
        clearBit(bRooks, sq);
        blackScore += ROOK_VAL + rookPST[sq ^ 56];
    }

    // 5. Queens
    Bitboard wQueens = pieces[static_cast<int>(Piece::WhiteQueen)];
    while (wQueens) {
        int sq = __builtin_ctzll(wQueens);
        clearBit(wQueens, sq);
        whiteScore += QUEEN_VAL + queenPST[sq];
    }
    Bitboard bQueens = pieces[static_cast<int>(Piece::BlackQueen)];
    while (bQueens) {
        int sq = __builtin_ctzll(bQueens);
        clearBit(bQueens, sq);
        blackScore += QUEEN_VAL + queenPST[sq ^ 56];
    }

    // 6. King
    Bitboard wKing = pieces[static_cast<int>(Piece::WhiteKing)];
    if (wKing) {
        int sq = __builtin_ctzll(wKing);
        whiteScore += KING_VAL + kingPST[sq];
    }
    Bitboard bKing = pieces[static_cast<int>(Piece::BlackKing)];
    if (bKing) {
        int sq = __builtin_ctzll(bKing);
        blackScore += KING_VAL + kingPST[sq ^ 56];
    }

    int netScore = whiteScore - blackScore;
    return (sideToMove == Color::White) ? netScore : -netScore;
}

bool Board::hasNonPawnMaterial(Color side) const
{
    if (side == Color::White) {
        return (pieces[static_cast<int>(Piece::WhiteKnight)] |
                pieces[static_cast<int>(Piece::WhiteBishop)] |
                pieces[static_cast<int>(Piece::WhiteRook)] |
                pieces[static_cast<int>(Piece::WhiteQueen)]) != 0;
    } else {
        return (pieces[static_cast<int>(Piece::BlackKnight)] |
                pieces[static_cast<int>(Piece::BlackBishop)] |
                pieces[static_cast<int>(Piece::BlackRook)] |
                pieces[static_cast<int>(Piece::BlackQueen)]) != 0;
    }
}

void Board::makeNullMove(UndoState& undo)
{
    undo.enPassantSquare = enPassantSquare;
    undo.zobristKey = zobristKey;

    if (enPassantSquare != -1)
    {
        zobristKey ^= Zobrist::enPassantKeys[enPassantSquare % 8];
        enPassantSquare = -1;
    }

    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    zobristKey ^= Zobrist::sideKey;
}

void Board::unmakeNullMove(const UndoState& undo)
{
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    enPassantSquare = undo.enPassantSquare;
    zobristKey = undo.zobristKey;
}

// ============================================================================
// ALPHA-BETA SEARCH ENGINE
// ============================================================================

constexpr int MATE_SCORE = 30000;
constexpr int INF_SCORE  = 100000;

int Board::quiescence(int alpha, int beta, int ply, std::uint64_t& nodesEvaluated)
{
    if (checkTime(nodesEvaluated)) return 0;
    nodesEvaluated++;

    // 1. Stand-Pat Evaluation (Baseline static score)
    int standPat = evaluate();

    // 2. Stand-Pat Beta Cutoff (Fail-hard)
    if (standPat >= beta)
    {
        return beta;
    }

    // 3. Stand-Pat Alpha Update
    if (standPat > alpha)
    {
        alpha = standPat;
    }

    // 4. Generate Legal Captures Only
    std::vector<Move> moves = generateLegalMoves();
    if (enableMoveOrdering) sortMoves(moves, 0, ply);

    for (Move move : moves)
    {
        // Only evaluate capture moves in Quiescence search
        if (!isMoveCapture(move))
        {
            continue;
        }

        UndoState undo = makeMove(move);

        int score = -quiescence(-beta, -alpha, ply + 1, nodesEvaluated);

        unmakeMove(move, undo);

        if (score >= beta)
        {
            return beta; // Cutoff
        }

        if (score > alpha)
        {
            alpha = score;
        }
    }

    return alpha;
}

int Board::negamax(int depth, int ply, int alpha, int beta, std::uint64_t& nodesEvaluated, TranspositionTable& tt, bool allowNull)
{
    if (checkTime(nodesEvaluated)) return 0;
    nodesEvaluated++;

    // 1. TT Probe
    Move ttMove = 0;
    const TTEntry* entry = tt.probe(zobristKey);
    if (entry != nullptr)
    {
        ttMove = entry->bestMove;
        if (entry->depth >= depth)
        {
            int ttScore = entry->score;
            if (ttScore > MATE_SCORE - 1000) ttScore -= ply;
            else if (ttScore < -MATE_SCORE + 1000) ttScore += ply;

            if (entry->flag == TTFlag::Exact)
                return ttScore;
            if (entry->flag == TTFlag::LowerBound && ttScore >= beta)
                return ttScore;
            if (entry->flag == TTFlag::UpperBound && ttScore <= alpha)
                return ttScore;
        }
    }

    bool inCheck = isKingInCheck(sideToMove);

    // 2. Leaf nodes transition into Quiescence search for tactical stability
    if (depth == 0)
    {
        // Detect terminal nodes before quiescence
        std::vector<Move> leafMoves = generateLegalMoves();
        if (leafMoves.empty())
        {
            if (inCheck) return -MATE_SCORE + ply;
            else return 0;
        }
        return quiescence(alpha, beta, ply, nodesEvaluated);
    }

    // 3. Null Move Pruning (NMP)
    if (enableNullMovePruning && allowNull && depth >= 3 && !inCheck && hasNonPawnMaterial(sideToMove))
    {
        UndoState undo;
        makeNullMove(undo);
        
        int R = 2; // Null Move Reduction
        int nullScore = -negamax(depth - 1 - R, ply + 1, -beta, -beta + 1, nodesEvaluated, tt, false);
        
        unmakeNullMove(undo);

        if (abortSearch) return 0;

        if (nullScore >= beta)
        {
            return beta;
        }
    }

    // 4. Generate legal moves
    std::vector<Move> moves = generateLegalMoves();

    // 5. Handle terminal checkmate & stalemate positions
    if (moves.empty())
    {
        if (inCheck)
        {
            // Checkmate: Return -MATE_SCORE + ply so shorter mate paths are preferred
            return -MATE_SCORE + ply;
        }
        else
        {
            // Stalemate: Return 0 (Draw score)
            return 0;
        }
    }

    if (enableMoveOrdering) sortMoves(moves, ttMove, ply);

    // 6. Negamax Search Loop with Alpha-Beta Pruning
    int originalAlpha = alpha;
    Move bestMove = moves[0];

    int movesSearched = 0;

    for (Move move : moves)
    {
        UndoState undo = makeMove(move);

        int score = 0;
        bool givesCheck = isKingInCheck(sideToMove);
        bool isCapture = isMoveCapture(move);
        bool isPromotion = (getMoveFlags(move) >= FlagKnightProm);

        // 6. Late Move Reductions (LMR)
        if (enableLateMoveReductions && depth >= 3 && movesSearched >= 3 && !inCheck && !givesCheck && !isCapture && !isPromotion)
        {
            int R = 1;
            if (movesSearched >= 6 && depth >= 4) R = 2;
            
            // Reduced depth zero-window search
            score = -negamax(depth - 1 - R, ply + 1, -alpha - 1, -alpha, nodesEvaluated, tt);
            
            // Re-search at full depth if LMR indicates move might be good
            if (score > alpha)
            {
                score = -negamax(depth - 1, ply + 1, -beta, -alpha, nodesEvaluated, tt);
            }
        }
        else
        {
            score = -negamax(depth - 1, ply + 1, -beta, -alpha, nodesEvaluated, tt);
        }

        unmakeMove(move, undo);
        movesSearched++;

        // Fail-hard beta cutoff
        if (score >= beta)
        {
            int storeScore = score;
            if (storeScore > MATE_SCORE - 1000) storeScore += ply;
            else if (storeScore < -MATE_SCORE + 1000) storeScore -= ply;
            
            tt.store(zobristKey, depth, storeScore, TTFlag::LowerBound, move);
            
            if (!isMoveCapture(move))
            {
                if (ply < 64 && killerMoves[ply][0] != move)
                {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;
                }
                Piece movingPiece = getPieceAt(getMoveFrom(move));
                historyTable[static_cast<int>(movingPiece)][getMoveTo(move)] += depth * depth;
            }
            
            return beta; // Cutoff: opponent would not allow this node
        }

        if (score > alpha)
        {
            alpha = score; // Found a better move for current side
            bestMove = move;
        }
    }

    int storeScore = alpha;
    if (storeScore > MATE_SCORE - 1000) storeScore += ply;
    else if (storeScore < -MATE_SCORE + 1000) storeScore -= ply;

    TTFlag flag = (alpha > originalAlpha) ? TTFlag::Exact : TTFlag::UpperBound;
    tt.store(zobristKey, depth, storeScore, flag, bestMove);

    return alpha;
}

bool Board::checkTime(std::uint64_t nodesEvaluated)
{
    if (abortSearch) return true;
    if (searchTimeLimitMs > 0 && (nodesEvaluated & 2047) == 0)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime).count();
        if (elapsed >= searchTimeLimitMs)
        {
            abortSearch = true;
            return true;
        }
    }
    return false;
}

int Board::getPieceValue(Piece piece) const
{
    switch (piece)
    {
        case Piece::WhitePawn:   case Piece::BlackPawn:   return 100;
        case Piece::WhiteKnight: case Piece::BlackKnight: return 300;
        case Piece::WhiteBishop: case Piece::BlackBishop: return 300;
        case Piece::WhiteRook:   case Piece::BlackRook:   return 500;
        case Piece::WhiteQueen:  case Piece::BlackQueen:  return 900;
        case Piece::WhiteKing:   case Piece::BlackKing:   return 10000;
        default: return 0;
    }
}

int Board::scoreMove(Move move, Move ttMove, int ply) const
{
    if (move == ttMove)
    {
        return 100000; // Search TT move absolutely first
    }

    if (isMoveCapture(move))
    {
        Piece attacker = getPieceAt(getMoveFrom(move));
        MoveFlag flags = getMoveFlags(move);
        Piece victim;

        if (flags == FlagEnPassant)
        {
            victim = (sideToMove == Color::White) ? Piece::BlackPawn : Piece::WhitePawn;
        }
        else
        {
            victim = getPieceAt(getMoveTo(move));
        }

        // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
        // E.g., PxQ = 9000 - 100 = 8900. KxP = 1000 - 10000 = -9000.
        // Base capture score 10000 to keep it above quiet moves.
        return 10000 + (getPieceValue(victim) * 10) - getPieceValue(attacker);
    }
    else
    {
        // 1st Killer Move
        if (ply < 64 && move == killerMoves[ply][0]) return 9000;
        // 2nd Killer Move
        if (ply < 64 && move == killerMoves[ply][1]) return 8000;

        // History Heuristic
        Piece movingPiece = getPieceAt(getMoveFrom(move));
        return historyTable[static_cast<int>(movingPiece)][getMoveTo(move)];
    }
}

void Board::sortMoves(std::vector<Move>& moves, Move ttMove, int ply) const
{
    int scores[256];
    int numMoves = static_cast<int>(moves.size());
    
    for (int i = 0; i < numMoves; ++i)
    {
        scores[i] = scoreMove(moves[i], ttMove, ply);
    }

    // In-place selection sort (zero allocations)
    for (int i = 0; i < numMoves - 1; ++i)
    {
        int bestIdx = i;
        int bestScore = scores[i];
        for (int j = i + 1; j < numMoves; ++j)
        {
            if (scores[j] > bestScore)
            {
                bestScore = scores[j];
                bestIdx = j;
            }
        }
        if (bestIdx != i)
        {
            std::swap(moves[i], moves[bestIdx]);
            std::swap(scores[i], scores[bestIdx]);
        }
    }
}

Move Board::searchRoot(int depth, int& outScore, std::uint64_t& outNodes, TranspositionTable& tt)
{
    outScore = -INF_SCORE;

    std::vector<Move> moves = generateLegalMoves();
    if (moves.empty())
        return 0; // No legal moves available

    Move ttMove = 0;
    const TTEntry* entry = tt.probe(zobristKey);
    if (entry != nullptr)
    {
        ttMove = entry->bestMove;
    }

    if (enableMoveOrdering) sortMoves(moves, ttMove, 0);

    Move bestMove = moves[0];
    int alpha = -INF_SCORE;
    int beta  = INF_SCORE;

    for (Move move : moves)
    {
        UndoState undo = makeMove(move);
        int score = -negamax(depth - 1, 1, -beta, -alpha, outNodes, tt);
        unmakeMove(move, undo);

        if (abortSearch)
            return 0; // Abort signal, discard this depth entirely

        if (score > alpha)
        {
            alpha = score;
            bestMove = move;
        }
    }

    outScore = alpha;
    return bestMove;
}

void Board::clearSearchHistory()
{
    for (int i = 0; i < 64; ++i)
    {
        killerMoves[i][0] = 0;
        killerMoves[i][1] = 0;
    }
    for (int i = 0; i < 12; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            historyTable[i][j] = 0;
        }
    }
}

Move Board::searchBestMove(int depth, int& outScore, std::uint64_t& outNodes)
{
    outNodes = 0;
    outScore = -INF_SCORE;
    
    abortSearch = false;
    searchTimeLimitMs = 0; // Fixed depth, no time limit

    clearSearchHistory();
    TranspositionTable tt;
    return searchRoot(depth, outScore, outNodes, tt);
}

Move Board::searchIterativeDeepening(int timeLimitMs, int& outScore, std::uint64_t& outNodes)
{
    outNodes = 0;
    outScore = -INF_SCORE;

    abortSearch = false;
    searchTimeLimitMs = timeLimitMs;
    searchStartTime = std::chrono::steady_clock::now();

    clearSearchHistory();
    TranspositionTable tt;
    Move globalBestMove = 0;

    // Fallback if time expires before depth 1 finishes
    std::vector<Move> legalMoves = generateLegalMoves();
    if (!legalMoves.empty())
        globalBestMove = legalMoves[0];

    for (int depth = 1; depth <= 64; ++depth)
    {
        int currentScore = -INF_SCORE;
        Move currentBestMove = searchRoot(depth, currentScore, outNodes, tt);

        if (abortSearch)
        {
            // Time expired during this depth. Discard incomplete results.
            // Retain globalBestMove and outScore from the PREVIOUS completed depth.
            break;
        }

        globalBestMove = currentBestMove;
        outScore = currentScore;
        
        // Optional: stop early if forced mate is found
        if (outScore >= (MATE_SCORE - 100) || outScore <= (-MATE_SCORE + 100))
        {
            break; 
        }
    }

    return globalBestMove;
}