/**
 * Массив из 400 клеток
 * Формула по индексам: index = y * cols + x
 */
#pragma once

#include "core/game.hpp"

#include <array>
#include <cstdint>

namespace ttt::my_player
{

    class SearchBoard
    {
    public:
        enum class Cell : std::uint8_t
        {
            EMPTY = 0,
            X = 1,
            O = 2,
            WALL = 3,
        };

        struct UndoInfo
        {
            int x = -1;
            int y = -1;
            Cell previous_cell = Cell::EMPTY;

            int previous_move_no = 0;
            game::Sign previous_current_player = game::Sign::NONE;
            game::Status previous_status = game::Status::CREATED;
            game::Sign previous_winner = game::Sign::NONE;
        };

        static constexpr int kCols = 20;
        static constexpr int kRows = 20;
        static constexpr int kCells = kCols * kRows;
        static constexpr int kWinLen = 5;

        SearchBoard();
        SearchBoard(const game::State &state, game::Sign my_sign);

        void load_from_state(const game::State &state, game::Sign my_sign);

        static constexpr bool inside(int x, int y)
        {
            return x >= 0 && x < kCols && y >= 0 && y < kRows;
        }

        static constexpr int index(int x, int y)
        {
            return y * kCols + x;
        }

        Cell get(int x, int y) const;
        void set_raw(int x, int y, Cell cell);

        bool is_empty(int x, int y) const;
        bool is_wall(int x, int y) const;
        bool is_stone(int x, int y) const;
        bool is_my_stone(int x, int y) const;
        bool is_opp_stone(int x, int y) const;

        game::Sign my_sign() const { return m_my_sign; }
        game::Sign opp_sign() const { return m_opp_sign; }
        game::Sign current_player() const { return m_current_player; }
        game::Sign winner() const { return m_winner; }
        game::Status status() const { return m_status; }
        int move_no() const { return m_move_no; }

        UndoInfo make_move(int x, int y, game::Sign sign);
        void undo_move(const UndoInfo &undo);

        static Cell cell_from_sign(game::Sign sign);
        static game::Sign sign_from_cell(Cell cell);
        static game::Sign opposite_sign(game::Sign sign);

    private:
        std::array<Cell, kCells> m_cells{};

        game::Sign m_my_sign = game::Sign::NONE;
        game::Sign m_opp_sign = game::Sign::NONE;
        game::Sign m_current_player = game::Sign::NONE;
        game::Sign m_winner = game::Sign::NONE;
        game::Status m_status = game::Status::CREATED;
        int m_move_no = 0;
    };

} // namespace ttt::my_player
