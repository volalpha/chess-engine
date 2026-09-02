#pragma once

#include "board.hpp"
#include <string>

void uciLoop();

namespace UCI {
    std::string moveToString(Move move);
    Move parseUciMove(Board& board, const std::string& moveStr);
}
