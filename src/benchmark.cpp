#include <iostream>
#include <chrono>
#include <vector>
#include "board.hpp"
#include "tt.hpp"
#include "zobrist.hpp"

int main() {
    initAllAttacks();
    Zobrist::init();

    constexpr int depth = 6;
    constexpr int iterations = 10;
    std::uint64_t totalNodes = 0;
    double totalSeconds = 0.0;

    for (int i = 0; i < iterations; ++i) {
        Board board;
        board.setStartingPosition();

        int score = 0;
        std::uint64_t nodes = 0;

        auto start = std::chrono::high_resolution_clock::now();
        board.searchBestMove(depth, score, nodes);
        auto end = std::chrono::high_resolution_clock::now();

        totalNodes += nodes;
        totalSeconds += std::chrono::duration<double>(end - start).count();
    }

    double nps = totalSeconds > 0 ? (totalNodes / totalSeconds) : 0;

    std::cout << "Search Depth : " << depth << "\n";
    std::cout << "Iterations   : " << iterations << "\n";
    std::cout << "Total Nodes  : " << totalNodes << "\n";
    std::cout << "Elapsed Time : " << totalSeconds << " seconds\n";
    std::cout << "NPS          : " << static_cast<std::uint64_t>(nps) << " nodes/sec\n";

    return 0;
}
