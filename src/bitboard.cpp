#include "bitboard.hpp"
#include <iostream>
#include <cstdlib>

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

// Reference ray-tracing used only during magic table initialization
static Bitboard slowRookAttacks(int square, Bitboard occupancy)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

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
            if (getBit(occupancy, targetSquare))
                break;
            r += dr[i];
            f += df[i];
        }
    }

    return attacks;
}

static Bitboard slowBishopAttacks(int square, Bitboard occupancy)
{
    Bitboard attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

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
            if (getBit(occupancy, targetSquare))
                break;
            r += dr[i];
            f += df[i];
        }
    }

    return attacks;
}

// Relevance mask: squares a blocker can occupy that affect the attack rays.
// Edge squares are excluded because a blocker on the board edge does not
// change the attack set (the ray terminates there regardless).
static Bitboard rookRelevanceMask(int square)
{
    Bitboard mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    for (int r = rank + 1; r < 7; ++r) mask |= (1ULL << (r * 8 + file));
    for (int r = rank - 1; r > 0; --r) mask |= (1ULL << (r * 8 + file));
    for (int f = file + 1; f < 7; ++f) mask |= (1ULL << (rank * 8 + f));
    for (int f = file - 1; f > 0; --f) mask |= (1ULL << (rank * 8 + f));

    return mask;
}

static Bitboard bishopRelevanceMask(int square)
{
    Bitboard mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    for (int r = rank + 1, f = file + 1; r < 7 && f < 7; ++r, ++f) mask |= (1ULL << (r * 8 + f));
    for (int r = rank + 1, f = file - 1; r < 7 && f > 0; ++r, --f) mask |= (1ULL << (r * 8 + f));
    for (int r = rank - 1, f = file + 1; r > 0 && f < 7; --r, ++f) mask |= (1ULL << (r * 8 + f));
    for (int r = rank - 1, f = file - 1; r > 0 && f > 0; --r, --f) mask |= (1ULL << (r * 8 + f));

    return mask;
}

static int popcount(Bitboard b) { return __builtin_popcountll(b); }

static Bitboard rookMagics[64];
static Bitboard bishopMagics[64];

static uint64_t prngState = 1070372;
static uint64_t randomUInt64()
{
    prngState ^= prngState >> 12;
    prngState ^= prngState << 25;
    prngState ^= prngState >> 27;
    return prngState * 2685821657736338717ULL;
}

static uint64_t randomUInt64FewBits()
{
    return randomUInt64() & randomUInt64() & randomUInt64();
}

static Bitboard findMagic(int sq, int shiftBits, bool isRook)
{
    Bitboard mask = isRook ? rookRelevanceMask(sq) : bishopRelevanceMask(sq);
    int numSubsets = 1 << popcount(mask);

    Bitboard subsets[4096];
    Bitboard attacks[4096];

    Bitboard subset = 0;
    int i = 0;
    do {
        subsets[i] = subset;
        attacks[i] = isRook ? slowRookAttacks(sq, subset) : slowBishopAttacks(sq, subset);
        i++;
        subset = (subset - mask) & mask;
    } while (subset);

    Bitboard table[4096];
    for (int attempt = 0; attempt < 100000000; ++attempt)
    {
        Bitboard magic = randomUInt64FewBits();
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6)
            continue;

        for (int j = 0; j < 4096; ++j) table[j] = 0ULL;

        bool failed = false;
        for (int j = 0; j < numSubsets; ++j)
        {
            int index = static_cast<int>((subsets[j] * magic) >> (64 - shiftBits));
            if (table[index] == 0ULL)
            {
                table[index] = attacks[j];
            }
            else if (table[index] != attacks[j])
            {
                failed = true;
                break;
            }
        }

        if (!failed)
            return magic;
    }
    return 0ULL;
}

static Bitboard rookMasks[64];
static Bitboard bishopMasks[64];
static int rookShifts[64];
static int bishopShifts[64];

// Maximum entries per square: rook 4096 (12 bits), bishop 512 (9 bits)
static Bitboard rookTable[64][4096];
static Bitboard bishopTable[64][512];

static void initSlidingAttacks()
{
    for (int sq = 0; sq < 64; ++sq)
    {
        rookMasks[sq] = rookRelevanceMask(sq);
        rookShifts[sq] = 64 - 12; // 12-bit table size (4096)

        rookMagics[sq] = findMagic(sq, 12, true);
        if (rookMagics[sq] == 0ULL)
        {
            std::cerr << "Failed to find rook magic for sq=" << sq << "\n";
            std::abort();
        }

        // Enumerate all subsets of the relevance mask to populate the table
        Bitboard mask = rookMasks[sq];
        Bitboard subset = 0;
        do {
            int index = static_cast<int>((subset * rookMagics[sq]) >> rookShifts[sq]);
            rookTable[sq][index] = slowRookAttacks(sq, subset);
            subset = (subset - mask) & mask;
        } while (subset);
    }

    for (int sq = 0; sq < 64; ++sq)
    {
        bishopMasks[sq] = bishopRelevanceMask(sq);
        bishopShifts[sq] = 64 - 9; // 9-bit table size (512)

        bishopMagics[sq] = findMagic(sq, 9, false);
        if (bishopMagics[sq] == 0ULL)
        {
            std::cerr << "Failed to find bishop magic for sq=" << sq << "\n";
            std::abort();
        }

        // Enumerate all subsets of the relevance mask to populate the table
        Bitboard mask = bishopMasks[sq];
        Bitboard subset = 0;
        do {
            int index = static_cast<int>((subset * bishopMagics[sq]) >> bishopShifts[sq]);
            bishopTable[sq][index] = slowBishopAttacks(sq, subset);
            subset = (subset - mask) & mask;
        } while (subset);
    }
}

// Retained for external callers that need raw ray-tracing (header-declared)
Bitboard maskRookAttacks(int square, Bitboard occupancy)
{
    return slowRookAttacks(square, occupancy);
}

Bitboard maskBishopAttacks(int square, Bitboard occupancy)
{
    return slowBishopAttacks(square, occupancy);
}

Bitboard getRookAttacks(int square, Bitboard occupancy)
{
    Bitboard blockers = occupancy & rookMasks[square];
    int index = static_cast<int>((blockers * rookMagics[square]) >> rookShifts[square]);
    return rookTable[square][index];
}

Bitboard getBishopAttacks(int square, Bitboard occupancy)
{
    Bitboard blockers = occupancy & bishopMasks[square];
    int index = static_cast<int>((blockers * bishopMagics[square]) >> bishopShifts[square]);
    return bishopTable[square][index];
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
    initSlidingAttacks();
}