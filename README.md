# C++ Chess Engine

A C++20-based UCI chess engine developed with a focus on correctness, performance, and clean architecture. The engine features Alpha-Beta search, Iterative Deepening, Null Move Pruning, Late Move Reductions, and a strict testing framework.

## Architecture & Features
* **Search:** Negamax with Alpha-Beta pruning, Quiescence Search, Iterative Deepening.
* **Evaluation:** Material counting and static evaluation.
* **Pruning & Reductions:** Null Move Pruning (NMP) with zugzwang safety, Late Move Reductions (LMR).
* **Move Ordering:** Principal Variation (PV), Transposition Table (TT) matches, Captures, Killer Moves, History Heuristics.
* **Transposition Table:** Zobrist hashing with exact, lower-bound, and upper-bound depth-preferred replacement.
* **Protocol:** Universal Chess Interface (UCI) compatible.

## Requirements
* CMake 3.16+
* C++20 compliant compiler (GCC/Clang)
* Pthreads support

## Build Instructions

To build the optimized engine and the testing suite:

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Running the Engine

Once built, you can run the executable natively. The engine operates purely as a UCI protocol endpoint, making it compatible with GUIs like Arena or CuteChess.

```bash
./build/chess_engine
```

**Manual Smoke Test:**
Type the following after starting the engine to verify basic function:
```text
uci
isready
position startpos
go depth 5
```
You should see `bestmove <move>`.

## Running the Tests

The regression suite relies on a standalone custom C++ testing harness built into the repository. Tests ensure all engine invariants (Zobrist hashing, TT Semantics, NMP/LMR correctness, UCI parsing) function as expected.

To run the standard regression suite:
```bash
cd build
ctest --output-on-failure
```
*Alternatively, you can run `./build/chess_engine_tests` directly.*

## Sanitizer Builds

To build the engine with AddressSanitizer (ASAN) and UndefinedBehaviorSanitizer (UBSAN), configure CMake with the `USE_SANITIZERS` flag. This configuration should be used for validation, but not for benchmarking, as sanitizers impact performance.

```bash
mkdir -p build_asan
cd build_asan
cmake -DUSE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
ctest --output-on-failure
```
