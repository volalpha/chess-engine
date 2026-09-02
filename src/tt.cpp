#include "tt.hpp"

TranspositionTable::TranspositionTable(std::size_t sizeInMB)
{
    std::size_t numEntries = (sizeInMB * 1024 * 1024) / sizeof(TTEntry);

    // Round down to nearest power of 2
    std::size_t power = 1;
    while (power * 2 <= numEntries)
        power *= 2;

    table.resize(power);
    mask = power - 1;
    clear();
}

void TranspositionTable::clear()
{
    for (auto& entry : table)
        entry = TTEntry{};
}

void TranspositionTable::store(std::uint64_t key, int depth, int score,
                               TTFlag flag, Move bestMove)
{
    TTEntry& entry = table[key & mask];

    if (entry.flag == TTFlag::None || depth >= entry.depth)
    {
        entry.key      = key;
        entry.score    = static_cast<std::int16_t>(score);
        entry.depth    = static_cast<std::uint8_t>(depth);
        entry.flag     = flag;
        entry.bestMove = bestMove;
    }
}

const TTEntry* TranspositionTable::probe(std::uint64_t key) const
{
    const TTEntry& entry = table[key & mask];

    if (entry.flag != TTFlag::None && entry.key == key)
        return &entry;

    return nullptr;
}
