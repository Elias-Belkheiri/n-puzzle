#include "Board.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ---- Constructors --------------------------------------------------------

Board::Board() : _size(0), _blank_pos(-1) {}

Board::Board(int size, const std::vector<int>& tiles)
	: _size(size), _blank_pos(-1), _tiles(tiles)
{
	for (int i = 0; i < static_cast<int>(_tiles.size()); ++i) {
		if (_tiles[i] == 0) {
			_blank_pos = i;
			break;
		}
	}
}

// ---- Neighbor generation -------------------------------------------------

std::vector<Board> Board::getNeighbors() const {
	std::vector<Board> neighbors;
	int row = _blank_pos / _size;
	int col = _blank_pos % _size;

	// Directions: up, down, left, right
	static const int dr[] = {-1, 1,  0, 0};
	static const int dc[] = { 0, 0, -1, 1};

	for (int d = 0; d < 4; ++d) {
		int nr = row + dr[d];
		int nc = col + dc[d];
		if (nr < 0 || nr >= _size || nc < 0 || nc >= _size)
			continue;

		std::vector<int> new_tiles(_tiles);
		int new_blank = nr * _size + nc;
		std::swap(new_tiles[_blank_pos], new_tiles[new_blank]);
		neighbors.emplace_back(_size, new_tiles);
	}
	return neighbors;
}

// ---- Comparison ----------------------------------------------------------

bool Board::operator==(const Board& other) const {
	return _tiles == other._tiles;
}

bool Board::operator!=(const Board& other) const {
	return !(*this == other);
}

// ---- Snail (spiral) goal state -------------------------------------------

Board Board::generateSnailGoal(int size) {
	std::vector<std::vector<int>> grid(size, std::vector<int>(size, -1));

	// Direction cycle: right, down, left, up
	static const int dr[] = {0, 1,  0, -1};
	static const int dc[] = {1, 0, -1,  0};

	int r = 0, c = 0, dir = 0;
	int total = size * size;

	for (int val = 1; val < total; ++val) {
		grid[r][c] = val;
		int nr = r + dr[dir];
		int nc = c + dc[dir];
		// Turn if next cell is out of bounds or already filled
		if (nr < 0 || nr >= size || nc < 0 || nc >= size || grid[nr][nc] != -1) {
			dir = (dir + 1) % 4;
			nr = r + dr[dir];
			nc = c + dc[dir];
		}
		r = nr;
		c = nc;
	}
	grid[r][c] = 0; // Blank goes in the last remaining cell

	// Flatten to 1‑D vector
	std::vector<int> tiles;
	tiles.reserve(total);
	for (int i = 0; i < size; ++i)
		for (int j = 0; j < size; ++j)
			tiles.push_back(grid[i][j]);

	return Board(size, tiles);
}

// ---- Pretty‑print -------------------------------------------------------

std::string Board::toString() const {
	int max_val = _size * _size - 1;
	int width   = (max_val > 0) ? static_cast<int>(std::log10(max_val)) + 1 : 1;

	std::ostringstream oss;
	for (int i = 0; i < _size; ++i) {
		for (int j = 0; j < _size; ++j) {
			if (j > 0) oss << ' ';
			oss << std::setw(width) << _tiles[i * _size + j];
		}
		oss << '\n';
	}
	return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Board& board) {
	os << board.toString();
	return os;
}

// ---- Hash (FNV‑1a) ------------------------------------------------------

size_t Board::Hash::operator()(const Board& b) const {
	size_t hash = 14695981039346656037ULL;
	for (int t : b._tiles) {
		hash ^= static_cast<size_t>(t);
		hash *= 1099511628211ULL;
	}
	return hash;
}
