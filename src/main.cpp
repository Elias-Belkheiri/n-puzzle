#include "Board.hpp"
#include "Parser.hpp"
#include "Generator.hpp"
#include "Solver.hpp"
#include "Heuristics.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <unistd.h>

// ---- ANSI colours (disabled if stdout is not a terminal) ----------------
static bool use_color = true;
static const char* BOLD   = "\033[1m";
static const char* GREEN  = "\033[32m";
static const char* CYAN   = "\033[36m";
static const char* YELLOW = "\033[33m";
static const char* RED    = "\033[31m";
static const char* RESET  = "\033[0m";

static void disableColor() {
	BOLD = GREEN = CYAN = YELLOW = RED = RESET = "";
	use_color = false;
}

// ---- Usage --------------------------------------------------------------

static void printUsage(const char* progname) {
	std::cout
		<< BOLD << "Usage: " << RESET << progname
		<< " [options] [input_file | -]\n\n"
		<< BOLD << "Options:\n" << RESET
		<< "  -s, --search <type>       Search algorithm: "
		<< CYAN << "astar" << RESET << " (default), "
		<< CYAN << "uniform" << RESET << ", "
		<< CYAN << "greedy" << RESET << "\n"
		<< "  -h, --heuristic <func>    Heuristic function: "
		<< CYAN << "manhattan" << RESET << " (default), "
		<< CYAN << "linear_conflict" << RESET << ", "
		<< CYAN << "hamming" << RESET << "\n"
		<< "  -g, --generate <N>        Generate an NxN puzzle\n"
		<< "  -S, --solvable            Force generated puzzle to be solvable\n"
		<< "  -U, --unsolvable          Force generated puzzle to be unsolvable\n"
		<< "  -i, --iterations <N>      Random walk moves from goal state\n"
		<< "  --help                    Show this help message\n\n"
		<< BOLD << "Examples:\n" << RESET
		<< "  " << progname << " puzzle.txt\n"
		<< "  " << progname << " - < puzzle.txt\n"
		<< "  " << progname << " -g 3 -h linear_conflict\n"
		<< "  " << progname << " -g 4 -S -i 20 -h linear_conflict\n"
		<< "  " << progname << " -g 5 -s greedy -h linear_conflict\n";
}

// ---- Main ---------------------------------------------------------------

int main(int argc, char** argv) {
	// Disable colour if not a terminal
	if (!isatty(fileno(stdout)))
		disableColor();

	std::string   file;
	int           gen_size   = 0;
	bool          force_solv = false;
	bool          force_unsv = false;
	int           iterations = -1;
	SearchType    search     = SearchType::ASTAR;
	HeuristicType heuristic  = HeuristicType::MANHATTAN;

	// ---- Argument parsing ------------------------------------------------
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		if (arg == "--help") {
			printUsage(argv[0]);
			return 0;
		} else if (arg == "-s" || arg == "--search") {
			if (++i >= argc) { std::cerr << "Missing value for " << arg << '\n'; return 1; }
			try { search = parseSearch(argv[i]); }
			catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
		} else if (arg == "-h" || arg == "--heuristic") {
			if (++i >= argc) { std::cerr << "Missing value for " << arg << '\n'; return 1; }
			try { heuristic = parseHeuristic(argv[i]); }
			catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
		} else if (arg == "-g" || arg == "--generate") {
			if (++i >= argc) { std::cerr << "Missing value for " << arg << '\n'; return 1; }
			try {
				size_t idx = 0;
				gen_size = std::stoi(argv[i], &idx);
				if (idx != std::string(argv[i]).size() || gen_size < 2) {
					std::cerr << "Puzzle size must be an integer >= 2\n";
					return 1;
				}
				if (gen_size > 100) {
					std::cerr << "Puzzle size too large (max 100)\n";
					return 1;
				}
			} catch (...) {
				std::cerr << "Invalid puzzle size: '" << argv[i] << "'\n";
				return 1;
			}
		} else if (arg == "-S" || arg == "--solvable") {
			force_solv = true;
		} else if (arg == "-U" || arg == "--unsolvable") {
			force_unsv = true;
		} else if (arg == "-i" || arg == "--iterations") {
			if (++i >= argc) { std::cerr << "Missing value for " << arg << '\n'; return 1; }
			try {
				size_t idx = 0;
				iterations = std::stoi(argv[i], &idx);
				if (idx != std::string(argv[i]).size() || iterations < 0) {
					std::cerr << "Iterations must be a non-negative integer\n";
					return 1;
				}
			} catch (...) {
				std::cerr << "Invalid iterations value: '" << argv[i] << "'\n";
				return 1;
			}
		} else if (arg[0] == '-' && arg.size() > 1) {
			std::cerr << "Unknown option: " << arg << '\n';
			printUsage(argv[0]);
			return 1;
		} else {
			file = arg;
		}
	}

	if (file.empty() && gen_size == 0) {
		std::cerr << RED << "Error: " << RESET
		          << "provide an input file, '-' for stdin, or use -g <N> to generate a puzzle.\n\n";
		printUsage(argv[0]);
		return 1;
	}

	// ---- Build the board -------------------------------------------------
	Board board;
	try {
		if (!file.empty())
			board = parseFile(file);
		else
			board = generateRandom(gen_size, force_solv, force_unsv, iterations);
	} catch (const std::exception& e) {
		std::cerr << RED << "Error: " << RESET << e.what() << '\n';
		return 1;
	}

	int n = board.getSize();
	GoalInfo goal(n);

	// ---- Print configuration --------------------------------------------
	std::cout << BOLD << "═══════════════════════════════════════\n"
	          << "           N-Puzzle Solver\n"
	          << "═══════════════════════════════════════\n" << RESET;
	std::cout << CYAN << "Puzzle size : " << RESET << n << "x" << n << '\n';
	std::cout << CYAN << "Search      : " << RESET << searchName(search) << '\n';
	std::cout << CYAN << "Heuristic   : " << RESET << heuristicName(heuristic) << '\n';
	std::cout << '\n' << BOLD << "Initial state:\n" << RESET << board << '\n';
	std::cout << BOLD << "Goal state (snail):\n" << RESET << goal.goal_board << '\n';

	// ---- Solvability check ----------------------------------------------
	if (!isSolvable(board, goal)) {
		std::cout << RED << BOLD << "This puzzle is unsolvable.\n" << RESET;
		return 0;
	}

	// ---- Solve ----------------------------------------------------------
	std::cout << YELLOW << "Searching..." << RESET << std::flush;

	auto t0 = std::chrono::steady_clock::now();
	Solution sol = solve(board, goal, search, heuristic);
	auto t1 = std::chrono::steady_clock::now();
	double elapsed = std::chrono::duration<double>(t1 - t0).count();

	std::cout << "\r                \r"; // clear "Searching..."

	if (!sol.found) {
		// Should not happen after solvability check, but just in case
		std::cout << RED << "No solution found.\n" << RESET;
		return 1;
	}

	// ---- Output results -------------------------------------------------
	std::cout << BOLD << GREEN << "Solution found!\n" << RESET << '\n';

	// Print the solution path
	std::cout << BOLD << "Solution sequence (" << sol.moves << " moves):\n" << RESET;
	for (int i = 0; i < static_cast<int>(sol.path.size()); ++i) {
		std::cout << CYAN << "--- Step " << i << " ---\n" << RESET;
		std::cout << sol.path[i] << '\n';
	}

	// Statistics
	std::cout << BOLD << "═══════════════════════════════════════\n"
	          << "             Statistics\n"
	          << "═══════════════════════════════════════\n" << RESET;
	std::cout << CYAN << "Moves                : " << RESET << sol.moves << '\n';
	std::cout << CYAN << "Time complexity      : " << RESET << sol.time_complexity
	          << " states selected\n";
	std::cout << CYAN << "Space complexity     : " << RESET << sol.space_complexity
	          << " max states in memory\n";
	std::cout << CYAN << "Wall-clock time      : " << RESET
	          << std::fixed << elapsed << " s\n";

	return 0;
}
