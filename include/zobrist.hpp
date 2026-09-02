#pragma once

#include <cstdint>

class Board;

namespace Zobrist
{
    extern std::uint64_t pieceKeys[12][64];
    extern std::uint64_t sideKey;
    extern std::uint64_t castlingKeys[16];
    extern std::uint64_t enPassantKeys[8];

    void init();
    std::uint64_t computeFullKey(const Board& board);
}
