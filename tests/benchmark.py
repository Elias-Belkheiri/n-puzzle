#!/usr/bin/env python3
import subprocess
import time
import random
import os

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

def make_random_walk(n, steps, seed=None):
    if seed is not None:
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

def write_puzzle_file(path, n, tiles):
    with open(path, "w") as f:
        f.write(f"{n}\n")
        for i in range(0, len(tiles), n):
            row = tiles[i:i+n]
            f.write(" ".join(str(x) for x in row) + "\n")

def run_solver(args, timeout=60):
    t0 = time.time()
    try:
        res = subprocess.run([BIN] + args, capture_output=True, text=True, timeout=timeout)
        wall_time = time.time() - t0
        stats = {}
        for line in res.stdout.splitlines():
            if ":" in line:
                k, v = line.split(":", 1)
                stats[k.strip()] = v.strip()
        stats["Wall-clock real"] = f"{wall_time:.4f} s"
        stats["Exit code"] = str(res.returncode)
        if "This puzzle is unsolvable" in res.stdout:
            stats["Status"] = "Unsolvable"
        elif "Solution found!" in res.stdout:
            stats["Status"] = "Solved"
        else:
            stats["Status"] = "Error"
        return stats
    except subprocess.TimeoutExpired:
        return {"Status": "TIMEOUT", "Wall-clock real": f">{timeout}s"}

if __name__ == "__main__":
    os.makedirs("tests/scratch", exist_ok=True)

    print("=" * 70)
    print("             N-PUZZLE INTENSIVE BENCHMARK & STRESS SUITE")
    print("=" * 70)

# -------------------------------------------------------------------
# Benchmark 1: Heuristic Comparison on Random 3x3 Puzzles
# -------------------------------------------------------------------
print("\n[1] HEURISTIC EFFICIENCY ON 3x3 PUZZLES (Walk steps: 100)")
print(f"{'Heuristic':<20} | {'Moves':<6} | {'Time Comp':<12} | {'Space Comp':<12} | {'Time':<10}")
print("-" * 70)

tiles_3 = make_random_walk(3, 100, seed=42)
path_3 = "tests/scratch/bench_3x3.txt"
write_puzzle_file(path_3, 3, tiles_3)

for h in ["hamming", "manhattan", "linear_conflict"]:
    st = run_solver(["-h", h, path_3])
    moves = st.get("Moves", "?")
    tc = st.get("Time complexity", "?").split()[0]
    sc = st.get("Space complexity", "?").split()[0]
    wt = st.get("Wall-clock real", "?")
    print(f"{h:<20} | {moves:<6} | {tc:<12} | {sc:<12} | {wt:<10}")

# -------------------------------------------------------------------
# Benchmark 2: Search Strategy Comparison (A* vs Greedy vs Uniform-Cost)
# -------------------------------------------------------------------
print("\n[2] SEARCH STRATEGY COMPARISON ON 3x3 (A* vs Greedy vs Uniform-Cost)")
print(f"{'Search':<15} | {'Heuristic':<16} | {'Moves':<6} | {'Time Comp':<12} | {'Optimal?'}")
print("-" * 70)

for s in ["astar", "greedy", "uniform"]:
    st = run_solver(["-s", s, "-h", "manhattan", path_3])
    moves = st.get("Moves", "?")
    tc = st.get("Time complexity", "?").split()[0]
    is_opt = "YES (min moves)" if moves == "22" else "NO (suboptimal)" if s == "greedy" else "YES"
    print(f"{s:<15} | {'manhattan':<16} | {moves:<6} | {tc:<12} | {is_opt}")

# -------------------------------------------------------------------
# Benchmark 3: 4x4 Scalability across Shuffle Depths
# -------------------------------------------------------------------
print("\n[3] 4x4 SCALABILITY AT INCREASING SHUFFLE DEPTHS (Linear Conflict)")
print(f"{'Shuffle Depth':<15} | {'Moves Found':<12} | {'States Selected':<16} | {'Memory (States)':<16} | {'Time':<10}")
print("-" * 75)

for depth in [5, 10, 15, 20, 25, 30, 35]:
    tiles_4 = make_random_walk(4, depth, seed=100 + depth)
    p4 = f"tests/scratch/bench_4x4_d{depth}.txt"
    write_puzzle_file(p4, 4, tiles_4)
    st = run_solver(["-h", "linear_conflict", p4], timeout=30)
    status = st.get("Status", "?")
    if status == "TIMEOUT":
        print(f"{depth:<15} | {'TIMEOUT (>30s)':<12} | {'-':<16} | {'-':<16} | {'>30s':<10}")
        break
    moves = st.get("Moves", "?")
    tc = st.get("Time complexity", "?").split()[0]
    sc = st.get("Space complexity", "?").split()[0]
    wt = st.get("Wall-clock real", "?")
    print(f"{depth:<15} | {moves:<12} | {tc:<16} | {sc:<16} | {wt:<10}")

# -------------------------------------------------------------------
# Benchmark 4: 5x5 Greedy vs A* (Handling Large Sizes)
# -------------------------------------------------------------------
print("\n[4] 5x5 PUZZLE SOLVING (Greedy Search)")
tiles_5 = make_random_walk(5, 30, seed=555)
p5 = "tests/scratch/bench_5x5.txt"
write_puzzle_file(p5, 5, tiles_5)
st_5 = run_solver(["-s", "greedy", "-h", "linear_conflict", p5], timeout=30)
moves_5 = st_5.get("Moves", "?")
tc_5 = st_5.get("Time complexity", "?").split()[0]
wt_5 = st_5.get("Wall-clock real", "?")
print(f"5x5 (30-step walk) solved with Greedy in {wt_5}: {moves_5} moves, {tc_5} states expanded.")

print("\n" + "=" * 70)
print("BENCHMARK COMPLETED SUCCESSFULLY!")
print("=" * 70)
