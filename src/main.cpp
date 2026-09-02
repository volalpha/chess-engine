#include <iostream>
#include "bitboard.hpp"
#include "zobrist.hpp"
#include "uci.hpp"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    initAllAttacks();
    Zobrist::init();

    uciLoop();
    return 0;
}