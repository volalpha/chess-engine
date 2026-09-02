#include "bitboard.hpp"
#include <iostream>

void setBit(Bitboard& board, int square)
{
    board |= (1ULL << square);
}

bool getBit(Bitboard board, int square)
{
    return board & (1ULL << square);
}

void clearBit(Bitboard& board, int square)
{
    board &= ~(1ULL << square);
}

void printBoard(Bitboard board)
{
    for (int rank = 7; rank >= 0; --rank)
    {
        std::cout << rank + 1 << " ";

        for (int file = 0; file < 8; ++file)
        {
            int square = rank * 8 + file;

            if (getBit(board, square))
                std::cout << "X ";
            else
                std::cout << ". ";
        }

        std::cout << '\n';
    }

    std::cout << "\n  a b c d e f g h\n";
}

// Global lookup table for precomputed knight attacks on all 64 squares
static Bitboard knightAttacks[64];

Bitboard maskKnightAttacks(int square)
{
    Bitboard attacks = 0ULL;

    int rank = square / 8;
    int file = square % 8;

    // 8 possible L-shaped relative moves for a knight (rank offset, file offset)
    constexpr int dr[8] = { 2,  2, -2, -2,  1,  1, -1, -1 };
    constexpr int df[8] = { 1, -1,  1, -1,  2, -2,  2, -2 };

    for (int i = 0; i < 8; ++i)
    {
        int targetRank = rank + dr[i];
        int targetFile = file + df[i];

        // Bounds check: target square must remain strictly inside the 8x8 board limits
        if (targetRank >= 0 && targetRank < 8 && targetFile >= 0 && targetFile < 8)
        {
            int targetSquare = targetRank * 8 + targetFile;
            setBit(attacks, targetSquare);
        }
    }

    return attacks;
}

void initKnightAttacks()
{
    for (int square = 0; square < 64; ++square)
    {
        knightAttacks[square] = maskKnightAttacks(square);
    }
}

Bitboard getKnightAttacks(int square)
{
    return knightAttacks[square];
}

// Global lookup table for precomputed king attacks on all 64 squares
static Bitboard kingAttacks[64];

Bitboard maskKingAttacks(int square)
{
    Bitboard attacks = 0ULL;

    int rank = square / 8;
    int file = square % 8;

    // 8 possible surrounding directions for a king (rank offset, file offset)
    constexpr int dr[8] = { 1, -1,  0,  0,  1,  1, -1, -1 };
    constexpr int df[8] = { 0,  0, -1,  1,  1, -1,  1, -1 };

    for (int i = 0; i < 8; ++i)
    {
        int targetRank = rank + dr[i];
        int targetFile = file + df[i];

        // Bounds check: target square must remain strictly inside the 8x8 board limits
        if (targetRank >= 0 && targetRank < 8 && targetFile >= 0 && targetFile < 8)
        {
            int targetSquare = targetRank * 8 + targetFile;
            setBit(attacks, targetSquare);
        }
    }

    return attacks;
}

void initKingAttacks()
{
    for (int square = 0; square < 64; ++square)
    {
        kingAttacks[square] = maskKingAttacks(square);
    }
}

Bitboard getKingAttacks(int square)
{
    return kingAttacks[square];
}

// Global lookup table for precomputed pawn attacks: [0] = White, [1] = Black
static Bitboard pawnAttacks[2][64];

Bitboard maskPawnAttacks(int side, int square)
{
    Bitboard attacks = 0ULL;

    int rank = square / 8;
    int file = square % 8;

    // side == 0 (White pawns): attack diagonally up (+1 rank)
    if (side == 0)
    {
        if (rank + 1 < 8 && file - 1 >= 0)
            setBit(attacks, (rank + 1) * 8 + (file - 1));
        if (rank + 1 < 8 && file + 1 < 8)
            setBit(attacks, (rank + 1) * 8 + (file + 1));
    }
    // side == 1 (Black pawns): attack diagonally down (-1 rank)
    else
    {
        if (rank - 1 >= 0 && file - 1 >= 0)
            setBit(attacks, (rank - 1) * 8 + (file - 1));
        if (rank - 1 >= 0 && file + 1 < 8)
            setBit(attacks, (rank - 1) * 8 + (file + 1));
    }

    return attacks;
}

void initPawnAttacks()
{
    for (int square = 0; square < 64; ++square)
    {
        pawnAttacks[0][square] = maskPawnAttacks(0, square); // White
        pawnAttacks[1][square] = maskPawnAttacks(1, square); // Black
    }
}

Bitboard getPawnAttacks(int side, int square)
{
    return pawnAttacks[side][square];
}

// Sliding Attacks Implementation (Occupancy-aware ray casting)

Bitboard maskRookAttacks(int square, Bitboard occupancy)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // 4 orthogonal directions: Up (+r), Down (-r), Right (+f), Left (-f)
    constexpr int dr[4] = { 1, -1,  0,  0 };
    constexpr int df[4] = { 0,  0,  1, -1 };

    for (int i = 0; i < 4; ++i)
    {
        int r = rank + dr[i];
        int f = file + df[i];

        while (r >= 0 && r < 8 && f >= 0 && f < 8)
        {
            int targetSquare = r * 8 + f;
            setBit(attacks, targetSquare);

            // Ray stops upon hitting a blocker (blocker IS attacked, behind is NOT)
            if (getBit(occupancy, targetSquare))
                break;

            r += dr[i];
            f += df[i];
        }
    }

    return attacks;
}

Bitboard maskBishopAttacks(int square, Bitboard occupancy)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    // 4 diagonal directions: Up-Right, Up-Left, Down-Right, Down-Left
    constexpr int dr[4] = { 1,  1, -1, -1 };
    constexpr int df[4] = { 1, -1,  1, -1 };

    for (int i = 0; i < 4; ++i)
    {
        int r = rank + dr[i];
        int f = file + df[i];

        while (r >= 0 && r < 8 && f >= 0 && f < 8)
        {
            int targetSquare = r * 8 + f;
            setBit(attacks, targetSquare);

            // Ray stops upon hitting a blocker
            if (getBit(occupancy, targetSquare))
                break;

            r += dr[i];
            f += df[i];
        }
    }

    return attacks;
}

Bitboard getRookAttacks(int square, Bitboard occupancy)
{
    return maskRookAttacks(square, occupancy);
}

Bitboard getBishopAttacks(int square, Bitboard occupancy)
{
    return maskBishopAttacks(square, occupancy);
}

Bitboard getQueenAttacks(int square, Bitboard occupancy)
{
    return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}

// Move Representation Helpers

std::string squareToString(int square)
{
    int file = square % 8;
    int rank = square / 8;

    std::string str = "";
    str += static_cast<char>('a' + file);
    str += static_cast<char>('1' + rank);
    return str;
}

void printMove(Move move)
{
    int from = getMoveFrom(move);
    int to = getMoveTo(move);
    MoveFlag flags = getMoveFlags(move);

    std::cout << squareToString(from) << squareToString(to);

    // If move is a promotion, append promotion piece character (q, r, b, n)
    if (isMovePromotion(move))
    {
        switch (flags)
        {
            case FlagKnightProm:
            case FlagKnightPromCap:
                std::cout << 'n';
                break;
            case FlagBishopProm:
            case FlagBishopPromCap:
                std::cout << 'b';
                break;
            case FlagRookProm:
            case FlagRookPromCap:
                std::cout << 'r';
                break;
            case FlagQueenProm:
            case FlagQueenPromCap:
                std::cout << 'q';
                break;
            default:
                break;
        }
    }
}

void initAllAttacks()
{
    initKnightAttacks();
    initKingAttacks();
    initPawnAttacks();
}