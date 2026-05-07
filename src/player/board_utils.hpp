/**
 * @file board_utils.hpp
 * @brief Iinline-помощники для работы с координатами доски игрока.
 *
 * Файл собирает часто используемые операции, создание game::Point, проверку
 * пустой клетки, оценку близости к центру и перевод индекса расширенной доски
 * обратно в игровые координаты.
 *
 */

#pragma once

#include <cstdlib>
#include <cstdint>

#include "search_board.hpp"

namespace ttt::my_player
{

    inline game::Point make_point(int x, int y) noexcept
    {
        game::Point point;
        point.x = x;
        point.y = y;
        return point;
    }

    inline bool is_empty_cell(const SearchBoard &board, int x, int y) noexcept
    {
        return SearchBoard::is_within_board(x, y) &&
               board.get_cell(x, y) == SearchBoard::Cell::EMPTY;
    }

    inline int center_distance(int x, int y) noexcept
    {
        const int center_x = SearchBoard::kBoardWidth / 2;
        const int center_y = SearchBoard::kBoardHeight / 2;

        return std::abs(x - center_x) + std::abs(y - center_y);
    }

    inline Score center_bonus(int x, int y) noexcept
    {
        return EngineConfig::kCenterBonusBase - center_distance(x, y);
    }

    inline int board_x_from_index(std::uint16_t index) noexcept
    {
        const int padded_x = index % SearchBoard::kPaddedWidth;
        return padded_x - SearchBoard::kPadding;
    }

    inline int board_y_from_index(std::uint16_t index) noexcept
    {
        const int padded_y = index / SearchBoard::kPaddedWidth;
        return padded_y - SearchBoard::kPadding;
    }

}
