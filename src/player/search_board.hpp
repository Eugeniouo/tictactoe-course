#pragma once

#include <array>
#include <cstdint>

#include "core/game.hpp"
#include "engine_config.hpp"

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
            std::uint16_t index = 0;

            game::Sign previous_current_player = game::Sign::NONE;
            game::Status previous_game_status = game::Status::CREATED;
            game::Sign previous_winner = game::Sign::NONE;
        };

        static constexpr int kPadding = EngineConfig::kBoardPadding;

        static constexpr int kBoardWidth = EngineConfig::kBoardWidth;
        static constexpr int kBoardHeight = EngineConfig::kBoardHeight;
        static constexpr int kBoardCellCount = kBoardWidth * kBoardHeight;

        static constexpr int kWinLength = EngineConfig::kWinLength;

        static constexpr int kPaddedWidth = kBoardWidth + 2 * kPadding;
        static constexpr int kPaddedHeight = kBoardHeight + 2 * kPadding;
        static constexpr int kPaddedCellCount = kPaddedWidth * kPaddedHeight;

        static constexpr bool is_within_board(int x, int y) noexcept
        {
            return x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight;
        }

        static constexpr int to_index(int x, int y) noexcept
        {
            return (y + kPadding) * kPaddedWidth + (x + kPadding);
        }

        SearchBoard();

        SearchBoard(const SearchBoard &) = delete;
        SearchBoard &operator=(const SearchBoard &) = delete;

        SearchBoard(SearchBoard &&) = default;
        SearchBoard &operator=(SearchBoard &&) = default;

        void load_from_state(const game::State &state, game::Sign my_sign);

        Cell get_cell(int x, int y) const noexcept;

        [[nodiscard]] UndoInfo apply_move(int x, int y, game::Sign sign) noexcept;
        void undo_move(const UndoInfo &undo) noexcept;

        game::Sign my_sign() const noexcept { return m_my_sign; }
        game::Sign current_player() const noexcept { return m_current_player; }
        game::Sign winner() const noexcept { return m_winner; }
        game::Status game_status() const noexcept { return m_game_status; }
        int move_number() const noexcept { return m_move_number; }
        int max_move_count() const noexcept { return m_max_move_count; }

        int stone_count() const noexcept { return m_stone_count; }

        std::uint16_t stone_index_at(int position) const noexcept
        {
            return m_stone_indices[position];
        }

        std::uint64_t hash() const noexcept { return m_hash; }

        static Cell cell_from_game_sign(game::Sign sign) noexcept;
        static game::Sign opposite_player_sign(game::Sign sign) noexcept;

    private:
        static constexpr int kDirections[4] = {
            1,
            kPaddedWidth,
            kPaddedWidth + 1,
            kPaddedWidth - 1,
        };

        static constexpr bool is_stone_cell(Cell cell) noexcept
        {
            return cell == Cell::X || cell == Cell::O;
        }

        bool has_move_limit() const noexcept
        {
            return m_max_move_count > 0;
        }

        void reset_cells_with_walls() noexcept;

        void add_stone_index(int index) noexcept;

        int count_stones_in_direction(int start_index, int step, Cell target_cell) const noexcept;
        bool has_five_from_index(int index, Cell target_cell) const noexcept;

        void update_game_status_after_move(int index, Cell moved_cell) noexcept;

        std::array<Cell, kPaddedCellCount> m_cells{};

        std::array<std::uint16_t, kBoardCellCount> m_stone_indices{};
        int m_stone_count = 0;

        game::Sign m_my_sign = game::Sign::NONE;
        game::Sign m_current_player = game::Sign::NONE;
        game::Sign m_winner = game::Sign::NONE;
        game::Status m_game_status = game::Status::CREATED;

        int m_move_number = 0;
        int m_max_move_count = 0;

        std::uint64_t m_hash = 0;
    };

}
