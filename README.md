# C++ Chess Engine

A C++20-based UCI chess engine developed with a focus on correctness, measurable performance, and deterministic behavior. The engine features an Alpha-Beta search, strict regression testing, and a zero-dependency architecture designed with a dependency-free architecture.

## Highlights

- **Search Algorithm:** Negamax with Alpha-Beta pruning, Quiescence Search, and Iterative Deepening
- **Pruning & Reductions:** Null Move Pruning (NMP) with non-pawn material (zugzwang) safety, Late Move Reductions (LMR)
- **Move Ordering:** Principal Variation (PV) via Transposition Table, Captures (MVV-LVA), Killer Moves, History Heuristics
- **Transposition Table (TT):** Zobrist hashing with exact, lower-bound, and upper-bound depth-preferred replacement semantics
- **Protocol:** Universal Chess Interface (UCI) implementation

## Architecture

- **State Representation:** 64-bit Bitboards for all piece types and occupancies. Moves are packed into a compact 16-bit integer format.
- **Move Generation:** Pseudo-legal move generation with in-place `makeMove()` / `unmakeMove()` legality verification. An `UndoState` struct stores the previous state required to restore Zobrist keys, castling rights, en-passant state, and captured pieces in O(1) time.
- **Search Core:** Recursively implemented Negamax bounded by Alpha-Beta fail-hard cutoffs, transitioning directly into a capturing-only Quiescence search at leaf nodes to resolve tactical instability.
- **Interface:** UCI command handling with asynchronous search control and time-management interrupts.

## Build

The engine uses a standard, zero-dependency CMake configuration. To build a highly optimized Release binary:

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

## Running the Engine

The compiled engine operates purely as a UCI protocol endpoint. It does not possess a standalone graphical interface or interactive CLI.

```bash
./build/chess_engine
```

**Manual Smoke Test**
To verify communication, issue the following standard UCI commands:

```text
uci
isready
position startpos
go depth 5
```

The engine will compute and respond with `bestmove <move>`.

## Testing

The repository contains a custom, dependency-free C++ testing harness that runs 16 regression tests covering core engine mechanics.

```bash
cd build
ctest --output-on-failure
```
*(Currently validated: 16/16 tests passing, 100% success rate)*

The test suite actively verifies:
- **Zobrist Consistency:** Proving that incremental hash updates perfectly match full-board recomputations after complex `makeMove`/`unmakeMove` sequences.
- **TT Semantics:** Confirming depth-preferred replacement logic.
- **Tactical Correctness:** Solving forced mates (Mate-in-1, Mate-in-2, Scholar's Mate).
- **Pruning Safety:** Ensuring NMP is correctly suppressed in pawn-only endgames.
- **Perft:** Validating exact move generation node counts (Perft 1, 2, 3) against known mathematical constants.
- **UCI Parsing:** Validating FEN strings, standard algebraic notation, and promotion edge cases.

## Sanitizer Validation

The CMake environment natively supports AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) builds for rigorous memory safety validation.

```bash
mkdir -p build_asan
cd build_asan
cmake -DUSE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j$(nproc)
ctest --output-on-failure
```

*Sanitizer builds introduce significant runtime overhead and must not be used for benchmarking.*

## Benchmarking / Perft

The repository provides a standalone benchmarking executable designed to measure raw search performance (Nodes Per Second / NPS) independent of the UCI protocol overhead.

```bash
./build/chess_engine_benchmark
```

This executable launches a deterministic Depth 6 search from the starting position, allowing for strict, reproducible before/after baseline comparisons when testing low-level engine optimizations.

## UCI / GUI Compatibility

Because the engine natively speaks the Universal Chess Interface protocol, it can be connected directly to any standard UCI-compliant GUI (e.g., Arena, CuteChess, or BanksiaGUI) for tournament play or visual analysis.

## Project Structure

```text
.
├── CMakeLists.txt
├── README.md
├── build.sh
├── include/
│   ├── bitboard.hpp
│   ├── board.hpp
│   ├── tt.hpp
│   ├── uci.hpp
│   └── zobrist.hpp
├── src/
│   ├── benchmark.cpp
│   ├── bitboard.cpp
│   ├── board.cpp
│   ├── main.cpp
│   ├── tt.cpp
│   ├── uci.cpp
│   └── zobrist.cpp
└── tests/
    └── regression_tests.cpp
```

## Technical Notes

- **Search Memory:** The primary search algorithm operates primarily on the stack. Care has been taken to avoid unnecessary dynamic allocations (`new` or resizing `std::vector`) within `generateLegalMoves`, `negamax`, or `quiescence` to improve performance.
- **Depth-Preferred Replacement:** The Transposition Table resolves hash collisions by prioritizing entries researched at higher depths over newer, shallower evaluations.
- **Warning Compliance:** The codebase compiles strictly under `-Wall -Wextra -Wpedantic` without emitting warnings.

## Validation Summary

| Check | Result |
|---|---|
| Release build | Passed |
| Regression suite | 16/16 passed |
| ASan + UBSan build | Passed |
| Sanitized regression suite | Passed |
| Strict warning check | Passed |

## Design Philosophy / Scope

This project is engineered to prioritize correctness validated through tests, deterministic behavior, measurable performance, and a maintainable C++ architecture over feature bloat. It favors clean, tested bounds and rigorous regression tracking, serving as a transparent foundation for modern C++ systems engineering.
