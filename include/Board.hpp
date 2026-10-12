#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>
#include <functional>
#include <iostream>

class Board {
public:
	Board();
	Board(int size, const std::vector<int>& tiles);

	int                    getSize() const { return _size; }
	int                    getTotalCells() const { return _size * _size; }
	int                    getBlankPos() const { return _blank_pos; }
	int                    at(int pos) const { return _tiles[pos]; }
	int                    at(int row, int col) const { return _tiles[row * _size + col]; }
	const std::vector<int>& getTiles() const { return _tiles; }

	// Returns all boards reachable by sliding one tile into the blank
	std::vector<Board>     getNeighbors() const;

	bool operator==(const Board& other) const;
	bool operator!=(const Board& other) const;

	// Generate the "snail" (spiral) goal state for an NxN puzzle
	static Board           generateSnailGoal(int size);

	std::string            toString() const;

	// Functor for use in unordered containers
	struct Hash {
		size_t operator()(const Board& b) const;
	};

private:
	int              _size;
	int              _blank_pos;
	std::vector<int> _tiles;
};

std::ostream& operator<<(std::ostream& os, const Board& board);
