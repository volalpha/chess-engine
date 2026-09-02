#pragma once

#include <vector>
#include <chrono>
#include "bitboard.hpp"

enum class Color
{
    White,
    Black
};

enum class Piece
{
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,

    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,

    Count
};

// Castling Rights Bitmasks
constexpr std::uint8_t WK_CASTLE = 1; // 0001 (White Kingside O-O)
constexpr std::uint8_t WQ_CASTLE = 2; // 0010 (White Queenside O-O-O)
constexpr std::uint8_t BK_CASTLE = 4; // 0100 (Black Kingside O-O)
constexpr std::uint8_t BQ_CASTLE = 8; // 1000 (Black Queenside O-O-O)
constexpr std::uint8_t ALL_CASTLE = 15; // 1111 (All castling rights)

struct UndoState
{
    Piece capturedPiece = Piece::Count;
    int enPassantSquare = -1;
    std::uint8_t castlingRights = 0;
    std::uint64_t zobristKey = 0;
};

class TranspositionTable;

class Board
{
private:
    Bitboard pieces[static_cast<int>(Piece::Count)];

    Bitboard whiteOccupancy;
    Bitboard blackOccupancy;
    Bitboard occupancy;

    Color sideToMove;

    std::uint8_t castlingRights;
    int enPassantSquare;
    std::uint64_t zobristKey;

    // Search & Time Management State
    volatile bool abortSearch;
    int searchTimeLimitMs;
    std::chrono::time_point<std::chrono::steady_clock> searchStartTime;

    // Move Ordering State
    Move killerMoves[64][2];
    int historyTable[12][64];

    bool checkTime(std::uint64_t nodesEvaluated);
    int getPieceValue(Piece piece) const;
    void clearSearchHistory();
    int scoreMove(Move move, Move ttMove, int ply) const;
    void sortMoves(std::vector<Move>& moves, Move ttMove, int ply) const;
    Move searchRoot(int depth, int& outScore, std::uint64_t& outNodes, TranspositionTable& tt);

public:
    bool enableMoveOrdering = true;
    bool enableNullMovePruning = true;
    bool enableLateMoveReductions = true;

    Board();

    void clearBoard();
    void setStartingPosition();
    void setupFen(const std::string& fen);
    void stopSearch() { abortSearch = true; }

    // Accessors
    Bitboard getPieceBitboard(Piece piece) const;
    Bitboard getOccupancy() const;
    Bitboard getWhiteOccupancy() const;
    Bitboard getBlackOccupancy() const;
    Color getSideToMove() const;
    std::uint8_t getCastlingRights() const;
    int getEnPassantSquare() const;
    std::uint64_t getZobristKey() const;

    // Piece query
    Piece getPieceAt(int square) const;

    // Setup helpers
    void setPiece(Piece piece, int square);
    void removePiece(Piece piece, int square);
    void setSideToMove(Color side);
    void setCastlingRights(std::uint8_t rights);
    void setEnPassantSquare(int square);
    void updateOccupancies();

    // Attack detection & check queries
    bool isSquareAttacked(int square, Color attackerColor) const;
    bool isKingInCheck(Color kingColor) const;
    bool hasNonPawnMaterial(Color side) const;

    // Move Generation
    std::vector<Move> generatePseudoLegalMoves() const;
    std::vector<Move> generateLegalMoves();

    // Static Evaluation
    int evaluate() const;

    // Exact state equality comparison
    bool operator==(const Board& other) const;

    // Make and Unmake Move
    UndoState makeMove(Move move);
    void unmakeMove(Move move, const UndoState& undo);
    void makeNullMove(UndoState& undo);
    void unmakeNullMove(const UndoState& undo);

    // Alpha-Beta Search & Quiescence
    int quiescence(int alpha, int beta, int ply, std::uint64_t& nodesEvaluated);
    int negamax(int depth, int ply, int alpha, int beta,
               std::uint64_t& nodesEvaluated, TranspositionTable& tt, bool allowNull = true);
    Move searchBestMove(int depth, int& outScore, std::uint64_t& outNodes);
    Move searchIterativeDeepening(int timeLimitMs, int& outScore, std::uint64_t& outNodes);
};