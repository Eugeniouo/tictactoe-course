#pragma once

#include "core/game.hpp"
#include "player/search_board.hpp"
#include <vector>

namespace ttt::my_player
{
    using MoveList = std::vector<game::Point>;

    MoveList generate_candidate_moves(const SearchBoard &board);

} // namespace ttt::my_player