#include "uci.hpp"
#include "board.hpp"
#include "bitboard.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

namespace {
    std::atomic<bool> searchRunning(false);
    std::thread searchThread;

    int parseSquare(const std::string& sqStr) {
        if (sqStr.length() < 2) return -1;
        int file = sqStr[0] - 'a';
        int rank = sqStr[1] - '1';
        if (file < 0 || file > 7 || rank < 0 || rank > 7) return -1;
        return rank * 8 + file;
    }
}

namespace UCI {
    std::string squareToString(int sq) {
        char file = 'a' + (sq % 8);
        char rank = '1' + (sq / 8);
        return std::string{file, rank};
    }

    std::string moveToString(Move move) {
        std::string s = squareToString(getMoveFrom(move)) + squareToString(getMoveTo(move));
        MoveFlag flags = getMoveFlags(move);
        if (flags == FlagKnightProm || flags == FlagKnightPromCap) s += 'n';
        else if (flags == FlagBishopProm || flags == FlagBishopPromCap) s += 'b';
        else if (flags == FlagRookProm || flags == FlagRookPromCap) s += 'r';
        else if (flags == FlagQueenProm || flags == FlagQueenPromCap) s += 'q';
        return s;
    }

    Move parseUciMove(Board& board, const std::string& moveStr) {
        if (moveStr.length() < 4) return 0;
        int from = parseSquare(moveStr.substr(0, 2));
        int to = parseSquare(moveStr.substr(2, 2));
        char prom = (moveStr.length() == 5) ? moveStr[4] : ' ';

        std::vector<Move> legalMoves = board.generateLegalMoves();
        for (Move move : legalMoves) {
            if (getMoveFrom(move) == from && getMoveTo(move) == to) {
                if (prom != ' ') {
                    MoveFlag flags = getMoveFlags(move);
                    bool isKnightProm = (flags == FlagKnightProm || flags == FlagKnightPromCap);
                    bool isBishopProm = (flags == FlagBishopProm || flags == FlagBishopPromCap);
                    bool isRookProm = (flags == FlagRookProm || flags == FlagRookPromCap);
                    bool isQueenProm = (flags == FlagQueenProm || flags == FlagQueenPromCap);

                    if (prom == 'n' && isKnightProm) return move;
                    if (prom == 'b' && isBishopProm) return move;
                    if (prom == 'r' && isRookProm) return move;
                    if (prom == 'q' && isQueenProm) return move;
                } else {
                    return move;
                }
            }
        }
        return 0; // Invalid or illegal move
    }
}

namespace {
    void doSearch(Board* board, int depth, int movetime) {
        searchRunning = true;
        int score = 0;
        std::uint64_t nodes = 0;
        Move bestMove = 0;

        if (movetime > 0) {
            bestMove = board->searchIterativeDeepening(movetime, score, nodes);
        } else {
            bestMove = board->searchBestMove(depth, score, nodes);
        }

        std::cout << "bestmove " << UCI::moveToString(bestMove) << std::endl;
        searchRunning = false;
    }
}

void uciLoop() {
    Board board;
    board.setStartingPosition();
    std::string line;

    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "uci") {
            std::cout << "id name C++ Chess Engine\n";
            std::cout << "id author Open Source\n";
            std::cout << "uciok\n" << std::flush;
        }
        else if (token == "isready") {
            std::cout << "readyok\n" << std::flush;
        }
        else if (token == "ucinewgame") {
            if (searchRunning) {
                board.stopSearch();
                if (searchThread.joinable()) searchThread.join();
            }
            board.setStartingPosition();
        }
        else if (token == "position") {
            if (searchRunning) {
                board.stopSearch();
                if (searchThread.joinable()) searchThread.join();
            }
            std::string type;
            ss >> type;
            if (type == "startpos") {
                board.setStartingPosition();
            } else if (type == "fen") {
                std::string fen, part;
                for (int i = 0; i < 6 && ss >> part; ++i) {
                    if (i > 0) fen += " ";
                    fen += part;
                }
                board.setupFen(fen);
            }

            std::string movesKeyword;
            ss >> movesKeyword;
            if (movesKeyword == "moves") {
                std::string moveStr;
                while (ss >> moveStr) {
                    Move m = UCI::parseUciMove(board, moveStr);
                    if (m != 0) {
                        board.makeMove(m);
                    }
                }
            }
        }
        else if (token == "go") {
            int depth = 5; // Default depth if none provided
            int movetime = 0;
            std::string param;
            while (ss >> param) {
                if (param == "depth") ss >> depth;
                else if (param == "movetime") ss >> movetime;
            }

            if (searchRunning) {
                board.stopSearch();
                if (searchThread.joinable()) {
                    searchThread.join();
                }
            }
            
            // Start search in a separate thread so UCI can still receive 'stop'
            searchThread = std::thread(doSearch, &board, depth, movetime);
        }
        else if (token == "stop") {
            board.stopSearch();
            if (searchThread.joinable()) {
                searchThread.join();
            }
        }
        else if (token == "quit") {
            board.stopSearch();
            if (searchThread.joinable()) {
                searchThread.join();
            }
            break;
        }
    }
}


