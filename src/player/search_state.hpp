#pragma once

#include <cstdint>

#include "core/game.hpp"
#include "incremental_evaluator.hpp"
#include "search_board.hpp"

namespace ttt::my_player
{

    class SearchState
    {
    public:
        struct Undo
        {
            SearchBoard::UndoInfo board_undo{};
        };

        SearchState() = default;

        void load_from_state(const game::State &state, game::Sign my_sign);

        [[nodiscard]] Undo apply_move(int x, int y, game::Sign sign) noexcept;
        void undo_move(const Undo &undo) noexcept;

        SearchBoard &board() noexcept { return m_board; }
        const SearchBoard &board() const noexcept { return m_board; }

        IncrementalEvaluator &evaluator() noexcept { return m_evaluator; }
        const IncrementalEvaluator &evaluator() const noexcept { return m_evaluator; }

        game::Sign winner() const noexcept
        {
            return m_board.winner();
        }

        game::Status game_status() const noexcept
        {
            return m_board.game_status();
        }

    private:
        SearchBoard m_board;
        IncrementalEvaluator m_evaluator;
    };

}
