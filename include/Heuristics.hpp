#pragma once

#include "Board.hpp"
#include <vector>
#include <functional>
#include <string>

// Precomputed goal‑position lookup table
struct GoalInfo {
	int              size;
	Board            goal_board;
	std::vector<int> goal_row;   // goal_row[tile_value] = row in goal
	std::vector<int> goal_col;   // goal_col[tile_value] = col  in goal
	std::vector<int> goal_pos;   // goal_pos[tile_value] = linear position in goal

	explicit GoalInfo(int size);
};

enum class HeuristicType {
	MANHATTAN,
	LINEAR_CONFLICT,
	HAMMING
};

using HeuristicFunc = std::function<int(const Board&, const GoalInfo&)>;

// --- Heuristic functions -------------------------------------------------
int manhattan(const Board& board, const GoalInfo& goal);
int linearConflict(const Board& board, const GoalInfo& goal);
int hamming(const Board& board, const GoalInfo& goal);

// --- Factories / helpers -------------------------------------------------
HeuristicFunc   getHeuristic(HeuristicType type);
std::string     heuristicName(HeuristicType type);
HeuristicType   parseHeuristic(const std::string& name);
