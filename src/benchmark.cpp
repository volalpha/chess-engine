#include <iostream>
#include <chrono>
#include <vector>
#include "board.hpp"
#include "tt.hpp"
#include "zobrist.hpp"

int main() {
    initAllAttacks();
    Zobrist::init();

    Board board;
    board.setStartingPosition();

    std::cout << "========================================\n";
    std::cout << "          PERFORMANCE BENCHMARK         \n";
    std::cout << "========================================\n\n";

    int depth = 6;
    int score = 0;
    std::uint64_t nodes = 0;

    auto start = std::chrono::high_resolution_clock::now();
    board.searchBestMove(depth, score, nodes);
    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0 ? (nodes / seconds) : 0;

    std::cout << "Search Depth : " << depth << "\n";
    std::cout << "Nodes Searched: " << nodes << "\n";
    std::cout << "Elapsed Time  : " << seconds << " seconds\n";
    std::cout << "NPS           : " << static_cast<std::uint64_t>(nps) << " nodes/sec\n";
    std::cout << "Best Score    : " << score << "\n\n";

    return 0;
}
