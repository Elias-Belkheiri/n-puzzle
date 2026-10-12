#pragma once

#include "Board.hpp"

// Generate an NxN puzzle
// If force_solvable is true, the board is guaranteed solvable.
// If force_unsolvable is true, the board is guaranteed unsolvable.
// If iterations > 0, performs a random walk of `iterations` moves from snail goal.
// With the default (-1), boards larger than 4 use a bounded 20-move walk.
Board generateRandom(int size, bool force_solvable = false, bool force_unsolvable = false, int iterations = -1);
