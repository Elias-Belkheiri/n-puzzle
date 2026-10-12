#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <set>

// Strip everything from '#' onward (comments) and trim whitespace
static std::string stripComment(const std::string& line) {
	auto pos = line.find('#');
	std::string stripped = (pos != std::string::npos) ? line.substr(0, pos) : line;
	// Trim leading/trailing whitespace
	size_t start = stripped.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = stripped.find_last_not_of(" \t\r\n");
	return stripped.substr(start, end - start + 1);
}

Board parseStream(std::istream& is, const std::string& source_name) {
	int size = 0;
	std::vector<int> tiles;
	bool size_read = false;

	std::string line;
	while (std::getline(is, line)) {
		std::string clean = stripComment(line);
		if (clean.empty()) continue;

		std::istringstream iss(clean);

		if (!size_read) {
			// First non‑comment, non‑empty line is the puzzle size
			if (!(iss >> size) || size < 2)
				throw std::runtime_error("Invalid puzzle size: " + clean);
			if (size > 100)
				throw std::runtime_error("Puzzle size too large (max 100)");
			// Make sure nothing else trails on this line
			std::string leftover;
			if (iss >> leftover)
				throw std::runtime_error(
					"Size line must contain only the puzzle dimension, got: " + clean);
			size_read = true;
			continue;
		}

		// Read tile values token by token to reject any non-integer or negative tokens
		std::string token;
		while (iss >> token) {
			size_t idx = 0;
			int val = 0;
			try {
				val = std::stoi(token, &idx);
				if (idx != token.size() || val < 0)
					throw std::runtime_error("");
			} catch (...) {
				throw std::runtime_error("Invalid tile value in file: '" + token + "'");
			}
			tiles.push_back(val);
		}
	}

	if (!size_read)
		throw std::runtime_error("No valid puzzle size found in " + source_name);

	int expected = size * size;
	if (static_cast<int>(tiles.size()) != expected)
		throw std::runtime_error(
			"Expected " + std::to_string(expected) + " tiles but got "
			+ std::to_string(tiles.size()));

	// Validate: tiles must be 0 .. N*N‑1 with no duplicates
	std::set<int> seen(tiles.begin(), tiles.end());
	if (static_cast<int>(seen.size()) != expected)
		throw std::runtime_error("Duplicate tile values detected");
	if (*seen.begin() != 0 || *seen.rbegin() != expected - 1)
		throw std::runtime_error(
			"Tile values must be in range [0, " + std::to_string(expected - 1) + "]");

	return Board(size, tiles);
}

Board parseFile(const std::string& filename) {
	if (filename == "-") {
		return parseStream(std::cin, "standard input");
	}
	std::ifstream file(filename);
	if (!file.is_open())
		throw std::runtime_error("Cannot open file: " + filename);
	return parseStream(file, "file: " + filename);
}
