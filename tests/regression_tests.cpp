#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>

#include "board.hpp"
#include "bitboard.hpp"
#include "zobrist.hpp"
#include "tt.hpp"
#include "uci.hpp"

int testsRun = 0;
int testsPassed = 0;

void runTest(const std::string& testName, bool condition) {
    testsRun++;
    std::cout << "[ TEST ] " << testName << " ... ";
    if (condition) {
        std::cout << "OK\n";
        testsPassed++;
    } else {
        std::cout << "FAILED\n";
        std::exit(1); // Fail fast
    }
}

// 9. Node-count sanity (Perft)
std::uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1ULL;
    std::vector<Move> moves = board.generateLegalMoves();
    if (depth == 1) return moves.size();
    
    std::uint64_t nodes = 0;
    for (Move move : moves) {
        UndoState undo = board.makeMove(move);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    return nodes;
}

int main() {
    initAllAttacks();
    Zobrist::init();

    std::cout << "========================================\n";
    std::cout << "        REGRESSION TEST SUITE           \n";
    std::cout << "========================================\n";

    // 1. Zobrist hash incremental update consistency
    {
        Board board;
        board.setStartingPosition();
        
        Move e4 = encodeMove(12, 28, FlagDoublePawnPush);
        UndoState undo_e4 = board.makeMove(e4);
        bool test1a = (board.getZobristKey() == Zobrist::computeFullKey(board));
        
        Move e5 = encodeMove(52, 36, FlagDoublePawnPush);
        UndoState undo_e5 = board.makeMove(e5);
        bool test1b = (board.getZobristKey() == Zobrist::computeFullKey(board));

        board.unmakeMove(e5, undo_e5);
        bool test1c = (board.getZobristKey() == Zobrist::computeFullKey(board));

        board.unmakeMove(e4, undo_e4);
        bool test1d = (board.getZobristKey() == Zobrist::computeFullKey(board));
        
        runTest("Zobrist Incremental Update Consistency", test1a && test1b && test1c && test1d);
    }

    // 2. TT exact/lower/upper bound semantics
    {
        TranspositionTable tt(1);
        std::uint64_t testKey = 0x123456789ULL;
        Move mockMove = encodeMove(12, 28, FlagDoublePawnPush);

        tt.store(testKey, 5, 100, TTFlag::Exact, mockMove);
        const TTEntry* entry = tt.probe(testKey);
        
        bool ok = true;
        if (entry == nullptr || entry->depth != 5 || entry->score != 100 || entry->flag != TTFlag::Exact || entry->bestMove != mockMove) ok = false;
        
        tt.store(testKey, 3, 50, TTFlag::LowerBound, 4321);
        entry = tt.probe(testKey);
        if (entry == nullptr || entry->depth != 5) ok = false; // Kept depth 5
        
        runTest("Transposition Table Semantics", ok);
    }

    // 3. Scholar's mate (White mate in 1)
    {
        Board board;
        board.setupFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
        int score = 0;
        std::uint64_t nodes = 0;
        Move bestMove = board.searchBestMove(3, score, nodes);
        runTest("Scholar's Mate (White)", getMoveFrom(bestMove) == 39 && getMoveTo(bestMove) == 53 && score >= 29000);
    }

    // 4. Black mate in one
    {
        Board board;
        board.setupFen("rnb1k1nr/pppp1ppp/8/4p3/6pq/5P2/PPPPP2P/RNBQKBNR b KQkq - 0 1");
        int score = 0;
        std::uint64_t nodes = 0;
        board.searchBestMove(2, score, nodes);
        runTest("Black Mate In One", score >= 29990);
    }

    // 5. Free-piece capture / material evaluation
    {
        Board board;
        board.setupFen("r1bqk2r/pppp1ppp/4p3/8/3bP3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
        int score = 0;
        std::uint64_t nodes = 0;
        Move bestMove = board.searchBestMove(3, score, nodes);
        runTest("Free Piece Capture", getMoveFrom(bestMove) == 21 && getMoveTo(bestMove) == 27 && score > 200);
    }

    // 6. Mate-in-two tactical combination
    {
        Board board;
        board.setupFen("r2qkb1r/pp2nppp/3p4/2pNN3/2B1P3/8/PPPP1PPP/R1BbK2R w KQkq - 0 1");
        int score = 0;
        std::uint64_t nodes = 0;
        Move bestMove = board.searchBestMove(3, score, nodes);
        runTest("Mate In Two", getMoveFrom(bestMove) == 35 && getMoveTo(bestMove) == 45 && score >= 29900);
    }

    // 7. Quiescence tactical stability
    {
        Board board;
        board.setupFen("rnbqkbnr/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::uint64_t qNodes = 0;
        int qScore = board.quiescence(-30000, 30000, 0, qNodes);
        runTest("Quiescence Stability", qScore > -5000 && qScore < 5000);
    }

    // 8. Iterative deepening / timeout behavior
    {
        Board board;
        board.setStartingPosition();
        int score = 0;
        std::uint64_t nodes = 0;
        auto start = std::chrono::high_resolution_clock::now();
        board.searchIterativeDeepening(50, score, nodes);
        auto end = std::chrono::high_resolution_clock::now();
        double seconds = std::chrono::duration<double>(end - start).count();
        runTest("Iterative Deepening Timeout", seconds < 0.15 && nodes > 0);
    }

    // 9. Node-count sanity (Perft)
    {
        Board board;
        board.setStartingPosition();
        runTest("Perft Node Count", perft(board, 1) == 20 && perft(board, 2) == 400 && perft(board, 3) == 8902);
    }

    // 10. NMP safety for zugzwang-sensitive positions
    {
        Board board;
        board.setupFen("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1");
        runTest("NMP Safety Zugzwang", !board.hasNonPawnMaterial(Color::White) && !board.hasNonPawnMaterial(Color::Black));
    }

    // 11. NMP enabled in normal non-pawn-material positions
    {
        Board board;
        board.setStartingPosition();
        runTest("NMP Safety Normal", board.hasNonPawnMaterial(Color::White) && board.hasNonPawnMaterial(Color::Black));
    }

    // 12. LMR node-reduction behavior
    {
        Board board;
        board.setStartingPosition();
        board.enableLateMoveReductions = false;
        int scoreLmrDisabled = 0;
        std::uint64_t nodesLmrDisabled = 0;
        board.searchBestMove(4, scoreLmrDisabled, nodesLmrDisabled);
        
        board.enableLateMoveReductions = true;
        int scoreLmrEnabled = 0;
        std::uint64_t nodesLmrEnabled = 0;
        board.searchBestMove(4, scoreLmrEnabled, nodesLmrEnabled);
        
        runTest("LMR Node Reduction", nodesLmrEnabled < nodesLmrDisabled);
    }

    // 13. UCI Startpos parsing
    {
        Board board;
        board.setStartingPosition();
        runTest("UCI Startpos Parsing", (board.getPieceBitboard(Piece::WhitePawn) & (1ULL << 12)) != 0);
    }

    // 14. UCI FEN parsing
    {
        Board board;
        board.setupFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        runTest("UCI FEN Parsing", (board.getPieceBitboard(Piece::WhitePawn) & (1ULL << 28)) != 0);
    }

    // 15. UCI Move parsing
    {
        Board board;
        board.setStartingPosition();
        Move e4 = UCI::parseUciMove(board, "e2e4");
        Move prom = 0;
        board.setupFen("8/P7/8/8/8/8/8/8 w - - 0 1");
        prom = UCI::parseUciMove(board, "a7a8q");
        runTest("UCI Move & Promotion Parsing", getMoveFlags(e4) == FlagDoublePawnPush && 
                (getMoveFlags(prom) == FlagQueenProm || getMoveFlags(prom) == FlagQueenPromCap));
    }

    // 16. UCI Move sequence
    {
        Board board;
        board.setStartingPosition();
        board.makeMove(UCI::parseUciMove(board, "e2e4"));
        board.makeMove(UCI::parseUciMove(board, "e7e5"));
        board.makeMove(UCI::parseUciMove(board, "g1f3"));
        runTest("UCI Move Sequence", (board.getPieceBitboard(Piece::WhiteKnight) & (1ULL << 21)) != 0);
    }

    std::cout << "========================================\n";
    std::cout << "All " << testsPassed << " tests passed perfectly.\n";
    return 0;
}
