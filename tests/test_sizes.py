#!/usr/bin/env python3
import subprocess
import time
import os
import sys

BIN = "./n-puzzle"

def generate_snail(n):
    grid = [[-1]*n for _ in range(n)]
    dr, dc = [0, 1, 0, -1], [1, 0, -1, 0]
    r, c, d = 0, 0, 0
    total = n * n
    for val in range(1, total):
        grid[r][c] = val
        nr, nc = r + dr[d], c + dc[d]
        if not (0 <= nr < n and 0 <= nc < n and grid[nr][nc] == -1):
            d = (d + 1) % 4
            nr, nc = r + dr[d], c + dc[d]
        r, c = nr, nc
    grid[r][c] = 0
    flat = []
    for row in grid:
        flat.extend(row)
    return flat

def make_walk(n, steps, seed=123):
    import random
    random.seed(seed)
    cur = generate_snail(n)
    for _ in range(steps):
        b = cur.index(0)
        br, bc = b // n, b % n
        moves = []
        for dr, dc in [(-1,0), (1,0), (0,-1), (0,1)]:
            nr, nc = br + dr, bc + dc
            if 0 <= nr < n and 0 <= nc < n:
                moves.append(nr * n + nc)
        nbr = random.choice(moves)
        cur[b], cur[nbr] = cur[nbr], cur[b]
    return cur

def write_puzzle(path, n, tiles):
    with open(path, "w") as f:
        f.write(f"{n}\n")
        for i in range(0, len(tiles), n):
            row = tiles[i:i+n]
            f.write(" ".join(str(x) for x in row) + "\n")

def run(args, timeout=20):
    t0 = time.time()
    try:
        res = subprocess.run([BIN] + args, capture_output=True, text=True, timeout=timeout)
        dt = time.time() - t0
        return res.returncode, res.stdout, res.stderr, dt
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT", timeout

os.makedirs("tests/scratch", exist_ok=True)

print("=" * 80)
print("      N-PUZZLE MULTI-SIZE EVALUATION SUITE (3, 4, 5, 6, 8, 10, 17, 20)")
print("=" * 80)

SIZES = [2, 3, 4, 5, 6, 8, 10, 17, 20]

print("\n--- Part A: Goal State Recognition (0-move solutions) ---")
print(f"{'Size (NxN)':<12} | {'Tiles':<8} | {'Status':<15} | {'Moves':<8} | {'Wall Time':<12}")
print("-" * 65)

for sz in SIZES:
    tiles = generate_snail(sz)
    p = f"tests/scratch/goal_{sz}.txt"
    write_puzzle(p, sz, tiles)
    code, out, err, dt = run([p])
    status = "PASS" if (code == 0 and "Moves                : 0" in out) else "FAIL"
    print(f"{f'{sz}x{sz}':<12} | {sz*sz:<8} | {status:<15} | {'0':<8} | {f'{dt*1000:.2f} ms':<12}")

print("\n--- Part B: Solvability & Parity Detection on Various Sizes ---")
print(f"{'Size (NxN)':<12} | {'Solvable Check':<18} | {'Unsolvable Check':<18} | {'Status'}")
print("-" * 65)

for sz in SIZES:
    # 1. Solvable instance (walk 10 steps)
    solv_tiles = make_walk(sz, 10, seed=sz*7)
    p_solv = f"tests/scratch/solv_{sz}.txt"
    write_puzzle(p_solv, sz, solv_tiles)
    code_s, out_s, _, _ = run([p_solv])
    solv_ok = (code_s == 0 and "Solution found!" in out_s)

    # 2. Unsolvable instance (swap 2 tiles)
    unsolv_tiles = list(solv_tiles)
    non_zeros = [i for i, x in enumerate(unsolv_tiles) if x != 0]
    unsolv_tiles[non_zeros[0]], unsolv_tiles[non_zeros[1]] = unsolv_tiles[non_zeros[1]], unsolv_tiles[non_zeros[0]]
    p_unsolv = f"tests/scratch/unsolv_{sz}.txt"
    write_puzzle(p_unsolv, sz, unsolv_tiles)
    code_u, out_u, _, _ = run([p_unsolv])
    unsolv_ok = (code_u == 0 and "This puzzle is unsolvable" in out_u)

    status = "PASS" if (solv_ok and unsolv_ok) else "FAIL"
    print(f"{f'{sz}x{sz}':<12} | {'CORRECT (Solvable)':<18} | {'CORRECT (Unsolv)':<18} | {status}")

print("\n--- Part C: Optimal A* Solving Across Sizes (Walk 5 to 15 moves) ---")
print(f"{'Size (NxN)':<12} | {'Walk Steps':<12} | {'Moves Found':<12} | {'States Selected':<18} | {'Time':<12}")
print("-" * 75)

for sz in [2, 3, 4, 5, 6, 8, 10, 17]:
    steps = 6 if sz <= 3 else 10 if sz <= 5 else 8
    tiles = make_walk(sz, steps, seed=sz*31)
    p = f"tests/scratch/solve_a_{sz}.txt"
    write_puzzle(p, sz, tiles)
    code, out, err, dt = run(["-h", "linear_conflict", p])
    moves = "?"
    states = "?"
    for line in out.splitlines():
        if "Moves                :" in line:
            moves = line.split(":")[1].strip()
        if "Time complexity      :" in line:
            states = line.split(":")[1].split()[0]
    print(f"{f'{sz}x{sz}':<12} | {steps:<12} | {moves:<12} | {states:<18} | {f'{dt*1000:.2f} ms':<12}")

print("\n--- Part D: Greedy Search on Large Sizes (Size 5, 8, 10, 17, 20) ---")
print(f"{'Size (NxN)':<12} | {'Walk Steps':<12} | {'Greedy Moves':<14} | {'States Selected':<18} | {'Time':<12}")
print("-" * 75)

for sz in [5, 8, 10, 17, 20]:
    steps = 15
    tiles = make_walk(sz, steps, seed=sz*43)
    p = f"tests/scratch/greedy_{sz}.txt"
    write_puzzle(p, sz, tiles)
    code, out, err, dt = run(["-s", "greedy", "-h", "linear_conflict", p])
    moves = "?"
    states = "?"
    for line in out.splitlines():
        if "Moves                :" in line:
            moves = line.split(":")[1].strip()
        if "Time complexity      :" in line:
            states = line.split(":")[1].split()[0]
    print(f"{f'{sz}x{sz}':<12} | {steps:<12} | {moves:<14} | {states:<18} | {f'{dt*1000:.2f} ms':<12}")

print("\n--- Part E: CLI Random Generation (-g N) for Sizes 3, 4, 5, 17 ---")
generation_failures = 0
for sz in [3, 4, 5, 17]:
    code, out, err, dt = run(["-g", str(sz), "-s", "greedy", "-h", "linear_conflict"])
    has_initial = "Initial state:" in out
    has_goal = "Goal state (snail):" in out
    result = "Unsolvable" if "unsolvable" in out else "Solved" if "Solution found!" in out else "Error"
    passed = code == 0 and has_initial and has_goal and result == "Solved"
    if not passed:
        generation_failures += 1
    print(f"  ./n-puzzle -g {sz:<2} => Board generated: {has_initial and has_goal} | Result: {result:<10} | Time: {dt*1000:.1f} ms | {'PASS' if passed else 'FAIL'}")

print("\n" + "=" * 80)
print("MULTI-SIZE EVALUATION COMPLETE: " + ("ALL SIZES VALIDATED!" if generation_failures == 0 else f"{generation_failures} GENERATION CASE(S) FAILED"))
print("=" * 80)
sys.exit(0 if generation_failures == 0 else 1)
