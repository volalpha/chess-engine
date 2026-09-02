#pragma once

#include <cstdint>
#include <vector>
#include "bitboard.hpp"

enum class TTFlag : std::uint8_t
{
    None,
    Exact,
    LowerBound,
    UpperBound
};

struct TTEntry
{
    std::uint64_t key   = 0;
    std::int16_t  score = 0;
    std::uint8_t  depth = 0;
    TTFlag        flag  = TTFlag::None;
    Move          bestMove = 0;
    std::uint16_t padding  = 0;
};
static_assert(sizeof(TTEntry) == 16, "TTEntry must be 16 bytes");

class TranspositionTable
{
public:
    explicit TranspositionTable(std::size_t sizeInMB = 16);

    void clear();
    void store(std::uint64_t key, int depth, int score, TTFlag flag, Move bestMove);
    const TTEntry* probe(std::uint64_t key) const;

private:
    std::vector<TTEntry> table;
    std::size_t mask;
};
