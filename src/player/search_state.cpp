/**
 * @file search_state.cpp
 * @brief Реализация синхронного обновления доски и оценщика.
 *
 * При загрузке состояния файл сначала обновляет SearchBoard, потом строит
 * инкрементальные оценки по этой доске. При ходе он применяет изменение к доске
 * и передает точный индекс клетки оценщику.
 */

#include "search_state.hpp"

#include <cassert>

namespace ttt::my_player
{

    void SearchState::load_from_state(const game::State &state, game::Sign my_sign)
    {
        m_board.load_from_state(state, my_sign);
        m_evaluator.load_from_board(m_board);
    }

    SearchState::Undo SearchState::apply_move(
        int x,
        int y,
        game::Sign sign) noexcept
    {
        assert(SearchBoard::is_within_board(x, y));
        assert(sign == game::Sign::X || sign == game::Sign::O);

        const SearchBoard::Cell cell = SearchBoard::cell_from_game_sign(sign);

        const SearchBoard::UndoInfo board_undo =
            m_board.apply_move(x, y, sign);

        m_evaluator.apply_move(board_undo.index, cell);

        return Undo{board_undo};
    }

    void SearchState::undo_move(const Undo &undo) noexcept
    {
        m_evaluator.undo_move(undo.board_undo.index);
        m_board.undo_move(undo.board_undo);
    }

}
