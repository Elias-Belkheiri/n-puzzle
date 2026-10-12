#include "Solver.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <stdexcept>

// =====================================================================
//  Solvability check (works for ANY goal state, including snail)
//
//  Solvable iff:
//    inversions(full_permutation)  ≡  manhattan(blank → goal_blank)  (mod 2)
//
//  where full_permutation includes the blank tile.
// =====================================================================

bool isSolvable(const Board& board, const GoalInfo& goal) {
	int n     = board.getSize();
	int total = n * n;

	// Build full permutation: perm[i] = goal position of tile at position i
	std::vector<int> perm(total);
	for (int i = 0; i < total; ++i)
		perm[i] = goal.goal_pos[board.at(i)];

	// Count inversions (O(n^2) – fine for N ≤ ~20)
	int inversions = 0;
	for (int i = 0; i < total; ++i)
		for (int j = i + 1; j < total; ++j)
			if (perm[i] > perm[j])
				++inversions;

	// Manhattan distance of blank from current to goal position
	int blank       = board.getBlankPos();
	int blank_goal  = goal.goal_pos[0];
	int blank_dist  = std::abs(blank / n - blank_goal / n)
	                + std::abs(blank % n - blank_goal % n);

	return (inversions % 2) == (blank_dist % 2);
}

// =====================================================================
//  A* / Uniform‑Cost / Greedy search
// =====================================================================

Solution solve(const Board& initial, const GoalInfo& goal,
               SearchType search, HeuristicType heuristic)
{
	HeuristicFunc h = getHeuristic(heuristic);

	// f(n) composition depending on search strategy
	auto computeF = [&](int g, int hVal) -> int {
		switch (search) {
			case SearchType::ASTAR:        return g + hVal;
			case SearchType::UNIFORM_COST: return g;
			case SearchType::GREEDY:       return hVal;
		}
		return g + hVal;
	};

	// --- Priority‑queue entry (min‑heap on f, tie‑breaking on higher g) ---
	struct PQEntry {
		int   f;
		int   g;
		Board board;
		bool operator>(const PQEntry& o) const {
			if (f != o.f)
				return f > o.f;
			return g < o.g; // Prefer larger g (closer to goal)
		}
	};
	using MinHeap = std::priority_queue<PQEntry,
	                                    std::vector<PQEntry>,
	                                    std::greater<PQEntry>>;

	struct NodeInfo {
		int  g;
		int  parent_blank; // blank position in parent board (-1 for initial)
		bool closed;
	};

	MinHeap                                              open;
	std::unordered_map<Board, NodeInfo, Board::Hash>     all_nodes;

	int time_complexity  = 0;
	int space_complexity = 0;

	// Seed the search
	int hStart = h(initial, goal);
	all_nodes[initial] = {0, -1, false};
	open.push({computeF(0, hStart), 0, initial});

	while (!open.empty()) {
		PQEntry entry = open.top();
		open.pop();
		const Board& current = entry.board;

		auto it = all_nodes.find(current);
		if (it != all_nodes.end() && it->second.closed)
			continue;

		++time_complexity;
		if (it != all_nodes.end())
			it->second.closed = true;

		space_complexity = std::max(space_complexity,
		                            static_cast<int>(all_nodes.size()));

		// Goal check
		if (current == goal.goal_board) {
			// Reconstruct path by undoing moves back to initial state
			std::vector<Board> path;
			Board state = current;
			while (true) {
				path.push_back(state);
				auto sit = all_nodes.find(state);
				if (sit == all_nodes.end() || sit->second.parent_blank == -1)
					break;
				// Recover parent: blank was swapped with tile at parent_blank
				int cur_blank = state.getBlankPos();
				int p_blank   = sit->second.parent_blank;
				std::vector<int> p_tiles = state.getTiles();
				std::swap(p_tiles[cur_blank], p_tiles[p_blank]);
				state = Board(state.getSize(), p_tiles);
			}
			std::reverse(path.begin(), path.end());

			return {true, path, static_cast<int>(path.size()) - 1,
			        time_complexity, space_complexity};
		}

		int current_g     = (it != all_nodes.end()) ? it->second.g : 0;
		int current_blank = current.getBlankPos();

		for (const Board& neighbor : current.getNeighbors()) {
			int tentative_g = current_g + 1;
			auto nit = all_nodes.find(neighbor);

			if (nit != all_nodes.end()) {
				if (nit->second.closed || tentative_g >= nit->second.g)
					continue;
				nit->second.g            = tentative_g;
				nit->second.parent_blank = current_blank;
			} else {
				all_nodes[neighbor] = {tentative_g, current_blank, false};
			}

			int hVal = h(neighbor, goal);
			open.push({computeF(tentative_g, hVal), tentative_g, neighbor});
		}
	}

	return {false, {}, 0, time_complexity, space_complexity};
}

// =====================================================================
//  Helpers
// =====================================================================

std::string searchName(SearchType type) {
	switch (type) {
		case SearchType::ASTAR:        return "A*";
		case SearchType::UNIFORM_COST: return "Uniform-cost";
		case SearchType::GREEDY:       return "Greedy best-first";
	}
	return "Unknown";
}

SearchType parseSearch(const std::string& name) {
	if (name == "astar")   return SearchType::ASTAR;
	if (name == "uniform") return SearchType::UNIFORM_COST;
	if (name == "greedy")  return SearchType::GREEDY;
	throw std::runtime_error("Unknown search type: " + name
		+ " (choices: astar, uniform, greedy)");
}
