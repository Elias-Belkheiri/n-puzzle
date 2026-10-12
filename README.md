# N-Puzzle Solver

An optimized, production-grade solver for the classic **N-Puzzle** ("taquin") game implemented in C++17, conforming to all 42 curriculum specifications and bonuses.

---

## Features

- **Search Algorithms** (Mandatory & Bonus):
  - **A\*** ($f(n) = g(n) + h(n)$): Optimal path finding with admissible heuristics.
  - **Uniform-Cost Search** ($f(n) = g(n)$): Guaranteed optimal path exploration.
  - **Greedy Best-First Search** ($f(n) = h(n)$): Rapid heuristic-guided search for large puzzle dimensions.
- **Admissible Heuristics** (User-selectable):
  - **Manhattan Distance** (Mandatory): $\sum |r - r_{goal}| + |c - c_{goal}|$ for all numbered tiles.
  - **Linear Conflict + Manhattan**: Strictly dominates Manhattan by identifying tiles in identical goal rows/columns that must cross each other, adding $+2$ moves per conflict without overestimating.
  - **Hamming Distance**: Count of misplaced tiles.
- **Dynamic Snail Solution**: Automatically generates the spiral target configuration for any $N \times N$ board.
- **Mathematical Solvability Check**: Instantaneous $O(N^2)$ parity verification using permutation inversions relative to the snail goal, preventing wasted compute on unsolvable boards.
- **Flexible Input & Generator**:
  - Parse input files with comments (`#`), irregular whitespace, and unaligned rows.
  - Standard input stream support via `-` (or pipes).
  - Built-in generator (`-g <N>`) with `--solvable`, `--unsolvable`, and `--iterations <N>` options.
- **Zero Memory Leaks**: Verified with AddressSanitizer and UndefinedBehaviorSanitizer.

---

## Compilation

```bash
make        # Compiles the executable ./n-puzzle
make clean  # Removes object files
make fclean # Removes object files and executable
make re     # Full recompilation
```

---

## Usage

```bash
./n-puzzle [options] [input_file | -]
```

### Options

| Flag | Description | Values / Defaults |
|------|-------------|-------------------|
| `-s`, `--search <type>` | Search strategy | `astar` (default), `uniform`, `greedy` |
| `-h`, `--heuristic <func>` | Heuristic function | `manhattan` (default), `linear_conflict`, `hamming` |
| `-g`, `--generate <N>` | Generate an $N \times N$ puzzle | Integer $\ge 2$ |
| `-S`, `--solvable` | Force generated board to be solvable | Boolean |
| `-U`, `--unsolvable` | Force generated board to be unsolvable | Boolean |
| `-i`, `--iterations <N>` | Random walk steps from goal state | Integer $\ge 0$ |
| `--help` | Display usage instructions | |

### Examples & Test Commands

#### 1. Testing Different Heuristics (Mandatory & Bonus)
All admissible heuristics guarantee the minimal path under A\*, but prune the search space differently:

```bash
# 1. Manhattan Distance (Mandatory - standard baseline)
./n-puzzle -h manhattan tests/puzzle-3-1.txt

# 2. Linear Conflict + Manhattan (Bonus - strictly dominates Manhattan, prunes 45%+ states)
./n-puzzle -h linear_conflict tests/puzzle-3-1.txt

# 3. Hamming Distance (Mandatory/Bonus - count of misplaced tiles, admissible but weak)
./n-puzzle -h hamming tests/puzzle-3-1.txt
```

*Empirical comparison on `tests/puzzle-3-1.txt`:*
| Heuristic | Moves | Time Complexity (States Selected) | Space Complexity (Max States) |
|---|:---:|:---:|:---:|
| `linear_conflict` | **15** | **33** | **58** |
| `manhattan` | **15** | **61** | **109** |
| `hamming` | **15** | **255** | **430** |

---

#### 2. Testing Different Search Algorithms (Bonus)
Explore the trade-offs between path optimality, exploration speed, and memory:

```bash
# A* Search (f = g + h) - Optimal path, balanced exploration
./n-puzzle -s astar tests/puzzle-3-1.txt

# Uniform-Cost Search (f = g) - Optimal path, cost-only expansion (Dijkstra-like BFS)
./n-puzzle -s uniform tests/puzzle-3-1.txt

# Greedy Best-First Search (f = h) - Rapid heuristic descent (may yield suboptimal path)
./n-puzzle -s greedy tests/puzzle-3-1.txt
```

*Search algorithm comparison on `tests/puzzle-3-1.txt`:*
| Search Type | Heuristic | Moves Found | Optimal? | Time Complexity |
|---|---|:---:|:---:|:---:|
| `astar` | `manhattan` | **15** | **YES** | 61 states |
| `uniform` | `manhattan` | **15** | **YES** | 8,407 states |
| `greedy` | `manhattan` | **39** | NO (suboptimal) | 161 states |

---

#### 3. Testing Puzzle Solvability & Parity
The solver rejects unsolvable boards instantaneously ($O(N^2)$) without performing any graph search:

```bash
# Test an unsolvable puzzle file (informs user and exits cleanly)
./n-puzzle tests/puzzle-3-unsolvable.txt

# Test an already-solved puzzle (returns in 0 moves)
./n-puzzle tests/puzzle-3-solved.txt

# Generate a guaranteed unsolvable board
./n-puzzle -g 3 -U
```

---

#### 4. Testing Generator & Large Puzzle Scaling (Bonus)
Use the built-in generator to test varying board sizes and shuffle depths:

```bash
# Generate and solve a guaranteed solvable 3x3 board
./n-puzzle -g 3 -S -h linear_conflict

# Generate a 4x4 puzzle with 20 shuffle iterations
./n-puzzle -g 4 -S -i 20 -h linear_conflict

# Solve a 5x5 board with Greedy search (instant solution)
./n-puzzle -g 5 -S -i 30 -s greedy -h linear_conflict

# Solve an 8x8 or 10x10 puzzle with Greedy search
./n-puzzle -g 10 -S -i 40 -s greedy -h linear_conflict

# Read puzzle via pipe / stdin
cat tests/puzzle-3-1.txt | ./n-puzzle -
```

---

#### 5. Automated Benchmark & Test Suites
The repository includes three automated Python test suites for defense evaluation:

```bash
# 1. Benchmark & Stress Suite: Side-by-side comparison of heuristics, algorithms & 4x4 scaling
python3 tests/benchmark.py

# 2. Multi-Size Evaluation Suite: Validates sizes from 2x2 up to 20x20
python3 tests/test_sizes.py

# 3. Robustness & Edge-Case Suite: Tests comments, whitespace, malformed files & parity
python3 tests/hard_test.py
```


---

## Output Metrics

According to the subject requirements, each run outputs:
1. **Initial and Goal States**: Visually aligned representations of the board.
2. **Solvability Status**: Reports if the board cannot reach the snail goal.
3. **Step-by-Step Solution**: Ordered sequence of states from step 0 to the final goal.
4. **Complexity in Time**: Total number of states ever selected in the opened set.
5. **Complexity in Size**: Maximum number of states ever represented in memory at the same time.
6. **Move Count**: Total number of tile transitions from start to goal.
7. **Execution Time**: High-resolution wall-clock duration.

---

## Mathematical Foundations for Defense

### 1. Solvability & Parity
The puzzle is solvable if and only if:
$$\text{inversions}(\pi) \equiv \text{manhattan}(\text{blank}, \text{blank}_{goal}) \pmod 2$$
where $\pi$ is the permutation mapping each tile to its target position in the snail goal.
- Horizontal blank moves swap adjacent elements in $\pi$, changing inversions by $\pm 1$ and Manhattan distance of blank by $\pm 1$.
- Vertical blank moves hop over $N - 1$ elements in $\pi$, making $2(N - 1) + 1$ adjacent transpositions (always odd), changing inversions by an odd amount and Manhattan distance by $\pm 1$.
- Thus, parity remains invariant under all valid moves.

### 2. Heuristic Admissibility
A heuristic $h(n)$ is admissible if $h(n) \le h^*(n)$ (it never overestimates the true cost):
- **Hamming**: Each move can place at most one misplaced tile into its goal position $\implies h(n) \le h^*(n)$.
- **Manhattan**: Each move alters the Manhattan distance of at most one tile by $1 \implies h(n) \le h^*(n)$.
- **Linear Conflict**: If two tiles in their goal row/col are reversed in order, at least one must move out of the line and back, requiring $\ge 2$ additional moves not counted by Manhattan $\implies h_{LC}(n) \le h^*(n)$ and $h_{LC}(n) \ge h_M(n)$ (monotonic dominance).
