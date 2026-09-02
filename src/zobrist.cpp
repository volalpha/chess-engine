#include "zobrist.hpp"
#include "board.hpp"

namespace Zobrist
{
    std::uint64_t pieceKeys[12][64];
    std::uint64_t sideKey;
    std::uint64_t castlingKeys[16];
    std::uint64_t enPassantKeys[8];

    // Deterministic 64-bit xorshift PRNG (fixed seed for reproducibility)
    static std::uint64_t prngState = 0x1234567890ABCDEFULL;

    static std::uint64_t nextRandom()
    {
        prngState ^= prngState << 13;
        prngState ^= prngState >> 7;
        prngState ^= prngState << 17;
        return prngState;
    }

    void init()
    {
        prngState = 0x1234567890ABCDEFULL;

        for (int piece = 0; piece < 12; ++piece)
            for (int square = 0; square < 64; ++square)
                pieceKeys[piece][square] = nextRandom();

        sideKey = nextRandom();

        for (int i = 0; i < 16; ++i)
            castlingKeys[i] = nextRandom();

        for (int i = 0; i < 8; ++i)
            enPassantKeys[i] = nextRandom();
    }

    std::uint64_t computeFullKey(const Board& board)
    {
        std::uint64_t key = 0;

        for (int piece = 0; piece < 12; ++piece)
        {
            Bitboard bb = board.getPieceBitboard(static_cast<Piece>(piece));
            while (bb)
            {
                int square = __builtin_ctzll(bb);
                key ^= pieceKeys[piece][square];
                bb &= bb - 1;
            }
        }

        if (board.getSideToMove() == Color::Black)
            key ^= sideKey;

        key ^= castlingKeys[board.getCastlingRights()];

        int ep = board.getEnPassantSquare();
        if (ep != -1)
            key ^= enPassantKeys[ep % 8];

        return key;
    }
}
