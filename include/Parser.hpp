#pragma once

#include "Board.hpp"
#include <string>
#include <istream>

Board parseFile(const std::string& filename);
Board parseStream(std::istream& is, const std::string& source_name = "stdin");
