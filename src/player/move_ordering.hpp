#pragma once

#include "move_generator.hpp"
#include "search_board.hpp"

namespace ttt::my_player
{
    void order_candidate_moves(SearchBoard &board, MoveList &moves);
}