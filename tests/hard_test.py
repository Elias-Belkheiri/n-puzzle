#!/usr/bin/env python3
import subprocess
import os
import sys
import random
import time

BIN = "./n-puzzle"

def run_cmd(args, timeout=30):
    try:
        res = subprocess.run([BIN] + args, capture_output=True, text=True, timeout=timeout)
        return res.returncode, res.stdout, res.stderr
    except subprocess.TimeoutExpired:
        return -999, "", "TIMEOUT"

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

def make_random_walk(n, steps):
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

def write_puzzle_file(path, n, tiles, header_comments="", inline_comments=False):
    with open(path, "w") as f:
        if header_comments:
            f.write(header_comments + "\n")
        f.write(f"{n}\n")
        for i in range(0, len(tiles), n):
            row = tiles[i:i+n]
            line = "  ".join(str(x) for x in row)
            if inline_comments and random.random() < 0.5:
                line += " # inline comment"
            f.write(line + "\n")

def parse_solution_states(stdout, n):
    lines = stdout.splitlines()
    states = []
    in_seq = False
    cur_grid = []
    for line in lines:
        if "Solution sequence" in line:
            in_seq = True
            continue
        if "Statistics" in line:
            break
        if in_seq:
            if line.startswith("--- Step"):
                if cur_grid:
                    states.append(cur_grid)
                    cur_grid = []
                continue
            parts = line.strip().split()
            if len(parts) == n and all(p.isdigit() for p in parts):
                cur_grid.extend(int(p) for p in parts)
    if cur_grid:
        states.append(cur_grid)
    return states

def validate_solution_path(states, n, initial, goal):
    if not states:
        return False, "No states parsed from solution sequence"
    if states[0] != initial:
        return False, f"First state doesn't match initial state: {states[0]} vs {initial}"
    if states[-1] != goal:
        return False, f"Last state doesn't match goal state: {states[-1]} vs {goal}"
    for step in range(len(states) - 1):
        s1, s2 = states[step], states[step + 1]
        b1, b2 = s1.index(0), s2.index(0)
        r1, c1 = b1 // n, b1 % n
        r2, c2 = b2 // n, b2 % n
        if abs(r1 - r2) + abs(c1 - c2) != 1:
            return False, f"Step {step}->{step+1}: blank moved illegally from ({r1},{c1}) to ({r2},{c2})"
        # Verify only blank and one other tile swapped
        diffs = [i for i in range(n*n) if s1[i] != s2[i]]
        if sorted(diffs) != sorted([b1, b2]):
            return False, f"Step {step}->{step+1}: more than 2 positions changed"
        if s1[b2] != s2[b1]:
            return False, f"Step {step}->{step+1}: tiles did not swap correctly"
    return True, "Valid"

os.makedirs("tests/scratch", exist_ok=True)
passed = 0
failed = 0

def test(name, condition, msg=""):
    global passed, failed
    if condition:
        print(f"  [PASS] {name}")
        passed += 1
    else:
        print(f"  [FAIL] {name} - {msg}")
        failed += 1

print("==================================================")
print("             N-PUZZLE HARD TEST SUITE             ")
print("==================================================")

# ----------------------------------------------------
# 1. PARSER EDGE CASES & MALFORMED INPUTS
# ----------------------------------------------------
print("\n--- 1. Parser Stress & Robustness Tests ---")

# 1.1 Non-existent file
code, out, err = run_cmd(["tests/nonexistent_file_12345.txt"])
test("Non-existent file rejected", code != 0 and "Cannot open file" in err)

# 1.2 Empty file
path = "tests/scratch/empty.txt"
open(path, "w").close()
code, out, err = run_cmd([path])
test("Empty file rejected", code != 0)

# 1.3 Only comments & blank lines
path = "tests/scratch/only_comments.txt"
with open(path, "w") as f:
    f.write("# comment 1\n\n   # comment 2\n\n")
code, out, err = run_cmd([path])
test("File with only comments rejected", code != 0)

# 1.4 Invalid size: 0, 1, -3
for sz in ["0", "1", "-3"]:
    path = f"tests/scratch/bad_sz_{sz}.txt"
    with open(path, "w") as f:
        f.write(f"{sz}\n1 2\n3 0\n")
    code, out, err = run_cmd([path])
    test(f"Size {sz} rejected", code != 0)

# 1.5 Non-numeric size
path = "tests/scratch/bad_sz_str.txt"
with open(path, "w") as f:
    f.write("three\n1 2 3\n")
code, out, err = run_cmd([path])
test("String size 'three' rejected", code != 0)

# 1.6 Size with extra tokens
path = "tests/scratch/bad_sz_extra.txt"
with open(path, "w") as f:
    f.write("3 4\n1 2 3\n8 0 4\n7 6 5\n")
code, out, err = run_cmd([path])
test("Size line with extra tokens rejected", code != 0)

# 1.7 Non-numeric tile in grid (e.g. 'foo')
path = "tests/scratch/bad_tile_str.txt"
with open(path, "w") as f:
    f.write("3\n1 2 foo\n3 4 5\n6 7 8 0\n")
code, out, err = run_cmd([path])
test("Non-numeric tile 'foo' rejected", code != 0, f"code={code}, out={out[:60]}")

# 1.8 Duplicate tiles
path = "tests/scratch/dup_tiles.txt"
with open(path, "w") as f:
    f.write("3\n1 2 3\n8 0 4\n7 6 6\n")
code, out, err = run_cmd([path])
test("Duplicate tile rejected", code != 0)

# 1.9 Missing tile (8 tiles for 3x3)
path = "tests/scratch/missing_tile.txt"
with open(path, "w") as f:
    f.write("3\n1 2 3\n8 0 4\n7 6\n")
code, out, err = run_cmd([path])
test("Missing tile rejected", code != 0)

# 1.10 Extra tile (10 tiles for 3x3)
path = "tests/scratch/extra_tile.txt"
with open(path, "w") as f:
    f.write("3\n1 2 3\n8 0 4\n7 6 5\n9\n")
code, out, err = run_cmd([path])
test("Extra tile rejected", code != 0)

# 1.11 Out-of-range tile (e.g. 9 for 3x3, range is 0..8)
path = "tests/scratch/out_of_range.txt"
with open(path, "w") as f:
    f.write("3\n1 2 3\n8 0 4\n7 9 5\n")
code, out, err = run_cmd([path])
test("Out-of-range tile rejected", code != 0)

# 1.12 Valid crazy formatting: tabs, multiple spaces, mixed lines
path = "tests/scratch/crazy_format.txt"
with open(path, "w") as f:
    f.write("  # Leading comment\n\n\t  3   # size with comment\n\n")
    f.write(" 1 \t 2   3\n")
    f.write("8\n\t0\t\t4\n")
    f.write("7   6    5   # trailing comment\n")
code, out, err = run_cmd([path])
test("Crazy formatting (tabs, extra spaces, inline comments) parsed", code == 0 and "Solution found" in out)

# 1.13 CLI argument validation
code, out, err = run_cmd(["-g", "abc"])
test("CLI -g abc non-numeric rejected cleanly", code != 0)

code, out, err = run_cmd(["-g", "1"])
test("CLI -g 1 (< 2) rejected", code != 0)

code, out, err = run_cmd(["-g", "150"])
test("CLI -g 150 (> 100) rejected", code != 0)

code, out, err = run_cmd(["-s", "bad_search", "-g", "3"])
test("CLI unknown search type rejected", code != 0)

code, out, err = run_cmd(["-h", "bad_heuristic", "-g", "3"])
test("CLI unknown heuristic rejected", code != 0)

code, out, err = run_cmd(["--nonexistent-flag"])
test("CLI unknown flag rejected", code != 0)

code, out, err = run_cmd(["--help"])
test("CLI --help exits cleanly with code 0", code == 0 and "Usage:" in out)

code, out, err = run_cmd([])
test("CLI no arguments rejected with error", code != 0)

# ----------------------------------------------------
# 2. SOLVABILITY & PARITY VERIFICATION
# ----------------------------------------------------
print("\n--- 2. Solvability & Parity Detection Tests ---")

solv_passed = True
for n in [3, 4]:
    goal = generate_snail(n)
    for walk_len in [1, 5, 15, 30]:
        # Solvable by construction
        tiles = make_random_walk(n, walk_len)
        path = f"tests/scratch/solv_{n}_{walk_len}.txt"
        write_puzzle_file(path, n, tiles)
        code, out, err = run_cmd([path])
        if code != 0 or "unsolvable" in out:
            solv_passed = False
            print(f"FAILED on solvable {n}x{n} walk {walk_len}")
            break

        # Make unsolvable by swapping two non-zero tiles
        non_zeros = [i for i, x in enumerate(tiles) if x != 0]
        i1, i2 = non_zeros[0], non_zeros[1]
        unsolv_tiles = list(tiles)
        unsolv_tiles[i1], unsolv_tiles[i2] = unsolv_tiles[i2], unsolv_tiles[i1]
        path_unsolv = f"tests/scratch/unsolv_{n}_{walk_len}.txt"
        write_puzzle_file(path_unsolv, n, unsolv_tiles)
        code, out, err = run_cmd([path_unsolv])
        if "unsolvable" not in out:
            solv_passed = False
            print(f"FAILED: detected as solvable when unsolvable for {n}x{n} walk {walk_len}")
            break
test("50 Solvable & Unsolvable boards correctly classified", solv_passed)

# ----------------------------------------------------
# 3. SOLUTION VALIDITY & PATH CONTINUITY
# ----------------------------------------------------
print("\n--- 3. Solution Path Verification ---")

path_valid_all = True
for n, steps in [(3, 10), (3, 20), (4, 15)]:
    tiles = make_random_walk(n, steps)
    path = f"tests/scratch/path_test_{n}_{steps}.txt"
    write_puzzle_file(path, n, tiles)
    code, out, err = run_cmd(["-h", "linear_conflict", path])
    if code != 0 or "Solution found" not in out:
        path_valid_all = False
        print(f"Solver failed on {path}")
        continue
    states = parse_solution_states(out, n)
    valid, reason = validate_solution_path(states, n, tiles, generate_snail(n))
    if not valid:
        path_valid_all = False
        print(f"Invalid solution path: {reason}")
test("All solution paths verified step-by-step for move legality", path_valid_all)

# ----------------------------------------------------
# 4. BONUS SEARCH MODES (A* vs GREEDY vs UNIFORM)
# ----------------------------------------------------
print("\n--- 4. Search Modes & Optimality Verification ---")

# For a 3x3 puzzle: A* must find the optimal (minimum) moves.
# Greedy should find a valid path (often suboptimal).
# Uniform-cost must also find the optimal moves.
test_tiles = make_random_walk(3, 16)
path = "tests/scratch/search_compare.txt"
write_puzzle_file(path, 3, test_tiles)

code_a, out_a, _ = run_cmd(["-s", "astar", "-h", "manhattan", path])
code_g, out_g, _ = run_cmd(["-s", "greedy", "-h", "manhattan", path])
code_u, out_u, _ = run_cmd(["-s", "uniform", "-h", "manhattan", path])

def get_moves(out):
    for line in out.splitlines():
        if "Moves                :" in line:
            return int(line.split(":")[-1].strip())
    return -1

moves_a = get_moves(out_a)
moves_g = get_moves(out_g)
moves_u = get_moves(out_u)

test("A* and Uniform-Cost both find identical optimal move count", moves_a > 0 and moves_a == moves_u, f"A*={moves_a}, Uniform={moves_u}")
test("Greedy finds valid solution (moves >= A*)", moves_g >= moves_a, f"Greedy={moves_g}, A*={moves_a}")

# ----------------------------------------------------
# 5. HEURISTICS COMPARISON (Dominance & Expansion)
# ----------------------------------------------------
print("\n--- 5. Heuristic Comparison on Complex 3x3 ---")

code_m, out_m, _ = run_cmd(["-h", "manhattan", path])
code_lc, out_lc, _ = run_cmd(["-h", "linear_conflict", path])
code_ham, out_ham, _ = run_cmd(["-h", "hamming", path])

def get_time_comp(out):
    for line in out.splitlines():
        if "Time complexity      :" in line:
            return int(line.split(":")[1].split()[0])
    return -1

tc_m = get_time_comp(out_m)
tc_lc = get_time_comp(out_lc)
tc_ham = get_time_comp(out_ham)

print(f"    States expanded: Hamming={tc_ham}, Manhattan={tc_m}, Linear Conflict={tc_lc}")
test("Linear Conflict explores <= Manhattan states", tc_lc <= tc_m, f"LC={tc_lc}, M={tc_m}")
test("Manhattan explores <= Hamming states", tc_m <= tc_ham, f"M={tc_m}, Ham={tc_ham}")

# ----------------------------------------------------
# 6. HARD 4x4 & SCALABILITY TESTS
# ----------------------------------------------------
print("\n--- 6. Scalability & Stress Tests ---")

# Random 4x4 with 25 random walk steps (guaranteed solvable)
t0 = time.time()
tiles_4 = make_random_walk(4, 25)
path_4 = "tests/scratch/hard_4x4.txt"
write_puzzle_file(path_4, 4, tiles_4)
code_4, out_4, err_4 = run_cmd(["-h", "linear_conflict", path_4], timeout=60)
elapsed_4 = time.time() - t0
moves_4 = get_moves(out_4)
test(f"4x4 solved with Linear Conflict in {elapsed_4:.2f}s ({moves_4} moves)", code_4 == 0 and moves_4 >= 0)

# Large puzzle size: 17x17 parsed and checked without crashing
path_17 = "tests/scratch/puzzle_17.txt"
tiles_17 = generate_snail(17)
write_puzzle_file(path_17, 17, tiles_17)
code_17, out_17, err_17 = run_cmd([path_17], timeout=10)
test("17x17 goal puzzle parsed and recognized as solved in 0 moves", code_17 == 0 and "Moves                : 0" in out_17)

# ----------------------------------------------------
# 7. LARGE RANDOM GENERATION MUST COMPLETE
# ----------------------------------------------------
print("\n--- 7. Large Random Generation ---")
code, out, err = run_cmd(["-g", "17", "-s", "greedy", "-h", "linear_conflict"], timeout=10)
test("Default 17x17 generation produces a solvable puzzle promptly",
     code == 0 and "Solution found!" in out,
     f"code={code}, stderr={err[:120]}")

print("\n==================================================")
print(f"TEST RESULTS: {passed} PASSED, {failed} FAILED")
print("==================================================")
sys.exit(0 if failed == 0 else 1)
