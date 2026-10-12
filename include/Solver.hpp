#pragma once

#include "Board.hpp"
#include "Heuristics.hpp"
#include <vector>
#include <string>

enum class SearchType {
	ASTAR,
	UNIFORM_COST,
	GREEDY
};

struct Solution {
	bool               found;
	std::vector<Board> path;
	int                moves;
	int                time_complexity;   // states popped from open set
	int                space_complexity;  // peak unique states in memory
};

Solution    solve(const Board& initial, const GoalInfo& goal,
                  SearchType search, HeuristicType heuristic);
bool        isSolvable(const Board& board, const GoalInfo& goal);
std::string searchName(SearchType type);
SearchType  parseSearch(const std::string& name);
