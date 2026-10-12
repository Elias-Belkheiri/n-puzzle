#include "Heuristics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// =====================================================================
//  GoalInfo – precompute tile → (row, col, pos) lookup table
// =====================================================================

GoalInfo::GoalInfo(int size)
	: size(size), goal_board(Board::generateSnailGoal(size))
{
	int total = size * size;
	goal_row.resize(total);
	goal_col.resize(total);
	goal_pos.resize(total);

	const auto& tiles = goal_board.getTiles();
	for (int i = 0; i < total; ++i) {
		int val       = tiles[i];
		goal_pos[val] = i;
		goal_row[val] = i / size;
		goal_col[val] = i % size;
	}
}

// =====================================================================
//  Manhattan distance
// =====================================================================

int manhattan(const Board& board, const GoalInfo& goal) {
	int n     = board.getSize();
	int total = n * n;
	int sum   = 0;
	int cur_row = 0;
	int cur_col = 0;
	for (int i = 0; i < total; ++i) {
		int tile = board.at(i);
		if (tile != 0) {
			sum += std::abs(cur_row - goal.goal_row[tile])
			     + std::abs(cur_col - goal.goal_col[tile]);
		}
		if (++cur_col == n) {
			cur_col = 0;
			++cur_row;
		}
	}
	return sum;
}

// =====================================================================
//  Hamming distance (count of misplaced tiles)
// =====================================================================

int hamming(const Board& board, const GoalInfo& goal) {
	int n     = board.getSize();
	int count = 0;
	for (int i = 0; i < n * n; ++i) {
		int tile = board.at(i);
		if (tile == 0) continue;
		if (goal.goal_pos[tile] != i)
			++count;
	}
	return count;
}

// =====================================================================
//  Linear Conflict + Manhattan
//
//  Two tiles are in *linear conflict* when they are both in their goal
//  row (or column) but in the wrong relative order.  Each such conflict
//  forces at least 2 extra moves on top of Manhattan.
//
//  We compute the minimum number of tiles to remove from each line so
//  that the remaining ones are in increasing goal‑position order.  This
//  equals  (count – LIS_length).
// =====================================================================

// Length of longest strictly increasing subsequence – O(n log n) with zero heap allocations
static int lisLength(const int* seq, int len) {
	if (len == 0) return 0;
	int tails[128];
	int tails_len = 0;
	for (int i = 0; i < len; ++i) {
		int x = seq[i];
		auto it = std::lower_bound(tails, tails + tails_len, x);
		if (it == tails + tails_len) {
			tails[tails_len++] = x;
		} else {
			*it = x;
		}
	}
	return tails_len;
}

int linearConflict(const Board& board, const GoalInfo& goal) {
	int n        = board.getSize();
	int conflict = 0;
	int seq[128];

	// Row conflicts
	for (int row = 0; row < n; ++row) {
		int len = 0;
		for (int col = 0; col < n; ++col) {
			int tile = board.at(row, col);
			if (tile != 0 && goal.goal_row[tile] == row) {
				if (len < 128) seq[len++] = goal.goal_col[tile];
			}
		}
		conflict += len - lisLength(seq, len);
	}

	// Column conflicts
	for (int col = 0; col < n; ++col) {
		int len = 0;
		for (int row = 0; row < n; ++row) {
			int tile = board.at(row, col);
			if (tile != 0 && goal.goal_col[tile] == col) {
				if (len < 128) seq[len++] = goal.goal_row[tile];
			}
		}
		conflict += len - lisLength(seq, len);
	}

	return manhattan(board, goal) + 2 * conflict;
}

// =====================================================================
//  Factory helpers
// =====================================================================

HeuristicFunc getHeuristic(HeuristicType type) {
	switch (type) {
		case HeuristicType::MANHATTAN:       return manhattan;
		case HeuristicType::LINEAR_CONFLICT: return linearConflict;
		case HeuristicType::HAMMING:         return hamming;
	}
	return manhattan;
}

std::string heuristicName(HeuristicType type) {
	switch (type) {
		case HeuristicType::MANHATTAN:       return "Manhattan distance";
		case HeuristicType::LINEAR_CONFLICT: return "Linear Conflict + Manhattan";
		case HeuristicType::HAMMING:         return "Hamming (Misplaced tiles)";
	}
	return "Unknown";
}

HeuristicType parseHeuristic(const std::string& name) {
	if (name == "manhattan")       return HeuristicType::MANHATTAN;
	if (name == "linear_conflict") return HeuristicType::LINEAR_CONFLICT;
	if (name == "hamming")         return HeuristicType::HAMMING;
	throw std::runtime_error("Unknown heuristic: " + name
		+ " (choices: manhattan, linear_conflict, hamming)");
}
