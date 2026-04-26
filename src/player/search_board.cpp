#include "search_board.hpp"

#include <cassert>

namespace ttt::my_player
{

    /**
     * @brief Создает внутреннюю поисковую доску из текущего состояния игры.
     *
     * @param state Состояние игры, полученное от движка.
     * @param my_sign Знак, которым играет наш бот в этой партии.
     */
    SearchBoard::SearchBoard(const game::State &state, game::Sign my_sign)
    {
        load_from_state(state, my_sign);
    }

    /**
     * @brief Копирует данные из ttt::game::State во внутреннее представление бота.
     *
     * @param state Состояние игры, полученное от движка.
     * @param my_sign Знак, которым играет наш бот.
     *
     * @note Метод синхронизирует
     * базовое состояние доски и служебные поля.
     */
    void SearchBoard::load_from_state(const game::State &state, game::Sign my_sign)
    {
        m_my_sign = my_sign;
        m_opponent_sign = opposite_player_sign(my_sign);
        m_current_player = state.get_current_player();
        m_winner = state.get_winner();
        m_game_status = state.get_status();
        m_move_number = state.get_move_no();
        m_max_move_count = state.get_opts().max_moves;

        for (int y = 0; y < kBoardHeight; ++y)
        {
            for (int x = 0; x < kBoardWidth; ++x)
            {
                m_cells[to_linear_index(x, y)] =
                    cell_from_game_sign(state.get_value(x, y));
            }
        }
    }

    /**
     * @brief Возвращает содержимое клетки.
     *
     * @param x Координата столбца.
     * @param y Координата строки.
     * @return Cell Значение клетки на доске.
     *
     * @note Выход за границы трактуется как WALL.
     */
    SearchBoard::Cell SearchBoard::get_cell(int x, int y) const
    {
        if (!is_within_board(x, y))
        {
            return Cell::WALL;
        }

        return m_cells[to_linear_index(x, y)];
    }

    void SearchBoard::set_cell(int x, int y, Cell cell)
    {
        assert(is_within_board(x, y));
        m_cells[to_linear_index(x, y)] = cell;
    }

    bool SearchBoard::is_cell_empty(int x, int y) const
    {
        return get_cell(x, y) == Cell::EMPTY;
    }

    bool SearchBoard::is_wall_cell(int x, int y) const
    {
        return get_cell(x, y) == Cell::WALL;
    }

    bool SearchBoard::contains_stone(int x, int y) const
    {
        const Cell cell = get_cell(x, y);
        return cell == Cell::X || cell == Cell::O;
    }

    bool SearchBoard::contains_my_stone(int x, int y) const
    {
        return get_cell(x, y) == cell_from_game_sign(m_my_sign);
    }

    bool SearchBoard::contains_opponent_stone(int x, int y) const
    {
        return get_cell(x, y) == cell_from_game_sign(m_opponent_sign);
    }

    /**
     * @brief Применяет ход к внутренней доске и сохраняет данные для отката.
     *
     * @param x Координата столбца.
     * @param y Координата строки.
     * @param sign Знак игрока, который делает ход.
     * @return UndoInfo Снимок состояния, достаточный для undo_move (отмены хода).
     *
     * @note Сейчас метод обновляет только базовое состояние доски и очередь
     * хода.
     */
    SearchBoard::UndoInfo SearchBoard::apply_move(int x, int y, game::Sign sign)
    {
        UndoInfo undo;
        undo.x = x;
        undo.y = y;
        undo.previous_cell = get_cell(x, y);
        undo.previous_move_number = m_move_number;
        undo.previous_max_move_count = m_max_move_count;
        undo.previous_current_player = m_current_player;
        undo.previous_game_status = m_game_status;
        undo.previous_winner = m_winner;

        assert(is_within_board(x, y));
        assert(undo.previous_cell == Cell::EMPTY);

        set_cell(x, y, cell_from_game_sign(sign));
        ++m_move_number;
        m_current_player = opposite_player_sign(sign);
        update_game_status_after_move(x, y, sign);

        return undo;
    }

    /**
     * @brief Откатывает один ранее примененный ход вместе со всей служебной
     * информацией.
     *
     * @param undo Данные, возвращенные методом apply_move.
     */
    void SearchBoard::undo_move(const UndoInfo &undo)
    {
        assert(is_within_board(undo.x, undo.y));

        m_cells[to_linear_index(undo.x, undo.y)] = undo.previous_cell;
        m_move_number = undo.previous_move_number;
        m_max_move_count = undo.previous_max_move_count;
        m_current_player = undo.previous_current_player;
        m_game_status = undo.previous_game_status;
        m_winner = undo.previous_winner;
    }

    /**
     * @brief Преобразует знак движка в тип клетки внутренней доски.
     *
     * @param sign Знак из ttt::game::Sign.
     * @return Cell значение внутреннего enum Cell.
     */
    SearchBoard::Cell SearchBoard::cell_from_game_sign(game::Sign sign)
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

    game::Sign SearchBoard::game_sign_from_cell(Cell cell)
    {
        switch (cell)
        {
        case Cell::X:
            return game::Sign::X;
        case Cell::O:
            return game::Sign::O;
        case Cell::WALL:
            return game::Sign::WALL;
        case Cell::EMPTY:
        default:
            return game::Sign::NONE;
        }
    }

    game::Sign SearchBoard::opposite_player_sign(game::Sign sign)
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

    /**
     * @brief Считает длину непрерывной цепочки камней в одном направлении.
     *
     * @param start_x координата x последнего хода
     * @param start_y координата y последнего хода
     * @param dx смещение по x для одного шага
     * @param dy смещение по y для одного шага
     * @param target_cell тип клетки, которую нужно посчитать
     *
     * @return количество подряд идущих клеток типа target_cell в одном направлении.
     *
     * @note стартовая клетка не включается в подсчет.
     */
    int SearchBoard::count_stones_in_direction(int start_x, int start_y, int dx, int dy, Cell target_cell) const
    {
        int count = 0;
        int x = start_x + dx;
        int y = start_y + dy;

        while (get_cell(x, y) == target_cell)
        {
            ++count;
            x += dx;
            y += dy;
        }

        return count;
    }

    /**
     * @brief Проверяет, создает ли последний ход линии длины 5.
     *
     * @param x координата x последнего хода
     * @param y координата y последнего хода
     * @param sign знак игрока, который только что сделал ход
     *
     * @return true - через последний ход создается линия длины 5
     *         false - через последний ход нет линии длины 5.
     */
    bool SearchBoard::has_five_from(int x, int y, game::Sign sign) const
    {
        const Cell target_cell = cell_from_game_sign(sign);

        if (!is_within_board(x, y))
        {
            return false;
        }

        if (target_cell != Cell::X && target_cell != Cell::O)
        {
            return false;
        }

        if (get_cell(x, y) != target_cell)
        {
            return false;
        }

        static constexpr int directions[4][2] = {
            {1, 0},
            {0, 1},
            {1, 1},
            {1, -1},
        };

        for (const auto &direction : directions)
        {
            const int dx = direction[0];
            const int dy = direction[1];

            const int total_length =
                1 + count_stones_in_direction(x, y, dx, dy, target_cell) +
                count_stones_in_direction(x, y, -dx, -dy, target_cell);

            if (total_length >= kWinLength)
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Обновляет состояние партии после уже примененного хода.
     *
     * @param x Координата x последнего хода
     * @param y Координата y последнего хода
     * @param sign Знак игрока, который только что сходил
     *
     * @note Метод повторяет логику ttt::game::State::process_move
     * для внутреннего поискового представления.
     */
    void SearchBoard::update_game_status_after_move(int x, int y, game::Sign sign)
    {
        const bool winning_move = has_five_from(x, y, sign);

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

            if (m_move_number >= m_max_move_count)
            {
                m_game_status = game::Status::ENDED;
                m_winner = opposite_player_sign(m_current_player);
                return;
            }

            m_game_status = game::Status::LAST_MOVE;
            return;
        }

        if (m_move_number >= m_max_move_count)
        {
            m_game_status = game::Status::ENDED;
            m_winner = game::Sign::NONE;
        }
    }

} // namespace ttt::my_player
