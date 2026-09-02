#pragma once

#include <cstdint>
#include <string>

using Bitboard = std::uint64_t;

void setBit(Bitboard& board, int square);
bool getBit(Bitboard board, int square);
void clearBit(Bitboard& board, int square);

void printBoard(Bitboard board);

// Global attack table initialization helper
void initAllAttacks();

void initKnightAttacks();
Bitboard maskKnightAttacks(int square);
Bitboard getKnightAttacks(int square);

void initKingAttacks();
Bitboard maskKingAttacks(int square);
Bitboard getKingAttacks(int square);

void initPawnAttacks();
Bitboard maskPawnAttacks(int side, int square);
Bitboard getPawnAttacks(int side, int square);

Bitboard maskRookAttacks(int square, Bitboard occupancy);
Bitboard maskBishopAttacks(int square, Bitboard occupancy);
Bitboard getRookAttacks(int square, Bitboard occupancy);
Bitboard getBishopAttacks(int square, Bitboard occupancy);
Bitboard getQueenAttacks(int square, Bitboard occupancy);

// Move Representation (Compact 16-bit uint16_t encoding)
// Bits 0-5  (6 bits): From square (0-63)
// Bits 6-11 (6 bits): To square (0-63)
// Bits 12-15 (4 bits): Special move flags (0-15)

using Move = std::uint16_t;

enum MoveFlag : std::uint16_t
{
    FlagQuiet           = 0,  // 0000: Normal move
    FlagDoublePawnPush  = 1,  // 0001: Double pawn push (e.g. e2-e4)
    FlagKingCastle      = 2,  // 0010: O-O
    FlagQueenCastle     = 3,  // 0011: O-O-O
    FlagCapture         = 4,  // 0100: Normal capture
    FlagEnPassant       = 5,  // 0101: En-passant capture

    FlagKnightProm      = 8,  // 1000: Knight promotion
    FlagBishopProm      = 9,  // 1001: Bishop promotion
    FlagRookProm        = 10, // 1010: Rook promotion
    FlagQueenProm       = 11, // 1011: Queen promotion

    FlagKnightPromCap   = 12, // 1100: Knight promotion with capture
    FlagBishopPromCap   = 13, // 1101: Bishop promotion with capture
    FlagRookPromCap     = 14, // 1110: Rook promotion with capture
    FlagQueenPromCap    = 15  // 1111: Queen promotion with capture
};

constexpr Move encodeMove(int from, int to, MoveFlag flags = FlagQuiet)
{
    return static_cast<Move>((from & 0x3F) | ((to & 0x3F) << 6) | ((static_cast<std::uint16_t>(flags) & 0x0F) << 12));
}

constexpr int getMoveFrom(Move move)
{
    return move & 0x3F;
}

constexpr int getMoveTo(Move move)
{
    return (move >> 6) & 0x3F;
}

constexpr MoveFlag getMoveFlags(Move move)
{
    return static_cast<MoveFlag>((move >> 12) & 0x0F);
}

constexpr bool isMoveCapture(Move move)
{
    return (move & (0x4 << 12)) != 0;
}

constexpr bool isMovePromotion(Move move)
{
    return (move & (0x8 << 12)) != 0;
}

std::string squareToString(int square);
void printMove(Move move);