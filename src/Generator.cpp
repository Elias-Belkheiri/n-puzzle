#include "Generator.hpp"
#include "Solver.hpp"
#include <algorithm>
#include <random>
#include <chrono>

static void swapTwoNonZero(std::vector<int>& tiles) {
	int first = -1, second = -1;
	for (int i = 0; i < static_cast<int>(tiles.size()); ++i) {
		if (tiles[i] != 0) {
			if (first == -1) first = i;
			else if (second == -1) { second = i; break; }
		}
	}
	if (first != -1 && second != -1) {
		std::swap(tiles[first], tiles[second]);
	}
}

Board generateRandom(int size, bool force_solvable, bool force_unsolvable, int iterations) {
	unsigned seed = static_cast<unsigned>(
		std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::mt19937 rng(seed);

	GoalInfo goal(size);

	// Full random permutations become impractical for search on large boards.
	// Keep the default large-board instance close to the goal via a legal walk.
	if (iterations == -1 && size > 4)
		iterations = 20;

	if (iterations > 0) {
		Board cur = goal.goal_board;
		for (int step = 0; step < iterations; ++step) {
			auto neighbors = cur.getNeighbors();
			std::uniform_int_distribution<size_t> dist(0, neighbors.size() - 1);
			cur = neighbors[dist(rng)];
		}
		if (force_unsolvable) {
			std::vector<int> tiles = cur.getTiles();
			swapTwoNonZero(tiles);
			return Board(size, tiles);
		}
		return cur;
	}

	std::vector<int> tiles = goal.goal_board.getTiles();
	std::shuffle(tiles.begin(), tiles.end(), rng);
	Board b(size, tiles);

	if (force_solvable && !isSolvable(b, goal)) {
		swapTwoNonZero(tiles);
		b = Board(size, tiles);
	} else if (force_unsolvable && isSolvable(b, goal)) {
		swapTwoNonZero(tiles);
		b = Board(size, tiles);
	}

	return b;
}
