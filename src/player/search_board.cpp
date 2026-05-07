/**
 * @file search_board.cpp
 * @brief Реализация загрузки, применения и отката ходов на поисковой доске.
 *
 * Файл создает padded доску с защитным рамками-стенами по краям, 
 * копирует состояние из игрового ядра, поддерживает список камней и обновляет 
 * хеш при каждом изменении.
 */

#include "search_board.hpp"
#include "zobrist.hpp"

#include <cassert>

namespace ttt::my_player
{

    SearchBoard::SearchBoard()
    {
        reset_cells_with_walls();
    }

    void SearchBoard::load_from_state(const game::State &state, game::Sign my_sign)
    {
        reset_cells_with_walls();

        m_my_sign = my_sign;

        m_current_player = state.get_current_player();
        m_winner = state.get_winner();
        m_game_status = state.get_status();

        m_move_number = state.get_move_no();
        m_max_move_count = state.get_opts().max_moves;

        m_stone_count = 0;

        for (int y = 0; y < kBoardHeight; ++y)
        {
            for (int x = 0; x < kBoardWidth; ++x)
            {
                const int index = to_index(x, y);
                const Cell cell = cell_from_game_sign(state.get_value(x, y));

                m_cells[index] = cell;

                if (is_stone_cell(cell))
                {
                    add_stone_index(index);
                }
            }
        }

        m_hash = 0;

        const Zobrist &zobrist = Zobrist::instance();

        for (int index = 0; index < kPaddedCellCount; ++index)
        {
            const Cell cell = m_cells[index];

            if (cell == Cell::X || cell == Cell::O)
            {
                m_hash ^= zobrist.cell_key(index, cell);
            }
        }

        m_hash ^= zobrist.current_player_key(m_current_player);
        m_hash ^= zobrist.status_key(m_game_status);
        m_hash ^= zobrist.winner_key(m_winner);
    }

    void SearchBoard::reset_cells_with_walls() noexcept
    {
        m_cells.fill(Cell::WALL);

        for (int y = 0; y < kBoardHeight; ++y)
        {
            for (int x = 0; x < kBoardWidth; ++x)
            {
                m_cells[to_index(x, y)] = Cell::EMPTY;
            }
        }
    }

    SearchBoard::Cell SearchBoard::get_cell(int x, int y) const noexcept
    {
        if (!is_within_board(x, y))
        {
            return Cell::WALL;
        }

        return m_cells[to_index(x, y)];
    }

    SearchBoard::UndoInfo SearchBoard::apply_move(int x, int y, game::Sign sign) noexcept
    {
        assert(is_within_board(x, y));
        assert(sign == game::Sign::X || sign == game::Sign::O);

        const int index = to_index(x, y);
        const Cell moved_cell = cell_from_game_sign(sign);

        assert(m_cells[index] == Cell::EMPTY);

        UndoInfo undo;
        undo.index = static_cast<std::uint16_t>(index);
        undo.previous_current_player = m_current_player;
        undo.previous_game_status = m_game_status;
        undo.previous_winner = m_winner;

        const Zobrist &zobrist = Zobrist::instance();

        m_hash ^= zobrist.current_player_key(m_current_player);
        m_hash ^= zobrist.status_key(m_game_status);
        m_hash ^= zobrist.winner_key(m_winner);

        m_cells[index] = moved_cell;
        m_hash ^= zobrist.cell_key(index, moved_cell);

        add_stone_index(index);

        ++m_move_number;

        m_current_player = opposite_player_sign(sign);

        update_game_status_after_move(index, moved_cell);

        m_hash ^= zobrist.current_player_key(m_current_player);
        m_hash ^= zobrist.status_key(m_game_status);
        m_hash ^= zobrist.winner_key(m_winner);

        return undo;
    }

    void SearchBoard::undo_move(const UndoInfo &undo) noexcept
    {
        const int index = undo.index;

        assert(index >= 0);
        assert(index < kPaddedCellCount);
        assert(is_stone_cell(m_cells[index]));

        const Zobrist &zobrist = Zobrist::instance();
        const Cell moved_cell = m_cells[index];

        m_hash ^= zobrist.current_player_key(m_current_player);
        m_hash ^= zobrist.status_key(m_game_status);
        m_hash ^= zobrist.winner_key(m_winner);

        m_hash ^= zobrist.cell_key(index, moved_cell);
        m_cells[index] = Cell::EMPTY;

        assert(m_stone_count > 0);
        assert(m_stone_indices[m_stone_count - 1] == undo.index);

        --m_stone_count;
        --m_move_number;

        m_current_player = undo.previous_current_player;
        m_game_status = undo.previous_game_status;
        m_winner = undo.previous_winner;

        m_hash ^= zobrist.current_player_key(m_current_player);
        m_hash ^= zobrist.status_key(m_game_status);
        m_hash ^= zobrist.winner_key(m_winner);
    }

    SearchBoard::Cell SearchBoard::cell_from_game_sign(game::Sign sign) noexcept
    {
        switch (sign)
        {
        case game::Sign::X:
            return Cell::X;
        case game::Sign::O:
            return Cell::O;
        case game::Sign::WALL:
            return Cell::WALL;
        case game::Sign::NONE:
        default:
            return Cell::EMPTY;
        }
    }

    game::Sign SearchBoard::opposite_player_sign(game::Sign sign) noexcept
    {
        switch (sign)
        {
        case game::Sign::X:
            return game::Sign::O;
        case game::Sign::O:
            return game::Sign::X;
        default:
            return game::Sign::NONE;
        }
    }

    void SearchBoard::add_stone_index(int index) noexcept
    {
        assert(m_stone_count >= 0);
        assert(m_stone_count < kBoardCellCount);

        m_stone_indices[m_stone_count] = static_cast<std::uint16_t>(index);
        ++m_stone_count;
    }

    int SearchBoard::count_stones_in_direction(
        int start_index,
        int step,
        Cell target_cell) const noexcept
    {
        int count = 0;
        int index = start_index + step;

        while (m_cells[index] == target_cell)
        {
            ++count;
            index += step;
        }

        return count;
    }

    bool SearchBoard::has_five_from_index(int index, Cell target_cell) const noexcept
    {
        assert(is_stone_cell(target_cell));

        for (const int step : kDirections)
        {
            const int total_length =
                1 +
                count_stones_in_direction(index, step, target_cell) +
                count_stones_in_direction(index, -step, target_cell);

            if (total_length >= kWinLength)
            {
                return true;
            }
        }

        return false;
    }

    void SearchBoard::update_game_status_after_move(int index, Cell moved_cell) noexcept
    {
        assert(is_stone_cell(moved_cell));

        const bool winning_move = has_five_from_index(index, moved_cell);

        if (m_game_status == game::Status::LAST_MOVE)
        {
            m_game_status = game::Status::ENDED;

            if (winning_move)
            {
                m_winner = game::Sign::NONE;
            }
            else
            {
                m_winner = m_current_player;
            }

            return;
        }

        m_game_status = game::Status::ACTIVE;
        m_winner = game::Sign::NONE;

        if (winning_move)
        {
            if (m_move_number % 2 == 0)
            {
                m_game_status = game::Status::ENDED;
                m_winner = opposite_player_sign(m_current_player);
                return;
            }

            if (has_move_limit() && m_move_number >= m_max_move_count)
            {
                m_game_status = game::Status::ENDED;
                m_winner = opposite_player_sign(m_current_player);
                return;
            }

            m_game_status = game::Status::LAST_MOVE;
            return;
        }

        if (has_move_limit() && m_move_number >= m_max_move_count)
        {
            m_game_status = game::Status::ENDED;
            m_winner = game::Sign::NONE;
        }
    }

}
