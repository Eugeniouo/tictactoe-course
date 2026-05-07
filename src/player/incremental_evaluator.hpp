#pragma once

#include <array>
#include <cstdint>

#include "core/game.hpp"
#include "search_board.hpp"
#include "threat_types.hpp"

namespace ttt::my_player
{

    class IncrementalEvaluator
    {
    public:
        static constexpr int kDirectionCount = 4;
        static constexpr int kSlotCount =
            SearchBoard::kPaddedCellCount * kDirectionCount;

        void load_from_board(const SearchBoard &board);

        void apply_move(std::uint16_t index, SearchBoard::Cell cell) noexcept;
        void undo_move(std::uint16_t index) noexcept;

        Score evaluate(game::Sign side) const noexcept;

        Score move_score(std::uint16_t index, game::Sign side) const noexcept;
        Score tactical_move_score(std::uint16_t index, game::Sign side) const noexcept;
        MoveClass move_class(std::uint16_t index, game::Sign side) const noexcept;

    private:
        inline static constexpr std::array<int, kDirectionCount> kDirections = {
            1,
            SearchBoard::kPaddedWidth,
            SearchBoard::kPaddedWidth + 1,
            SearchBoard::kPaddedWidth - 1,
        };

        static constexpr int slot_index(int index, int direction_id) noexcept
        {
            return index * kDirectionCount + direction_id;
        }

        static constexpr bool is_stone_cell(SearchBoard::Cell cell) noexcept
        {
            return cell == SearchBoard::Cell::X ||
                   cell == SearchBoard::Cell::O;
        }

        static int side_index(game::Sign side) noexcept;

        static bool is_playable_index(int index) noexcept;

        static DirectionThreat unpack_side_threat(
            std::uint8_t packed,
            int side) noexcept;

        static SearchBoard::Cell cell_for_side(game::Sign side) noexcept;
        static int tactical_line_value(
            SearchBoard::Cell cell,
            SearchBoard::Cell side_cell) noexcept;
        static Score tactical_window_score(const int line[9], int start) noexcept;
        static Score tactical_line_score(const int line[9]) noexcept;

        std::uint32_t encode_window(int index, int direction_id) const noexcept;
        Score compute_tactical_move_score(
            int index,
            SearchBoard::Cell side_cell) const noexcept;

        void recompute_directed_slot(int index, int direction_id) noexcept;

        void remove_line_slot(int slot) noexcept;
        void add_line_slot(int slot) noexcept;

        void recompute_move_cell(int index) noexcept;

        int collect_affected_cells(
            int index,
            std::array<std::uint16_t, 33> &out) const noexcept;

        static bool contains_cell(
            const std::array<std::uint16_t, 33> &cells,
            int count,
            std::uint16_t value) noexcept;

    private:
        std::array<SearchBoard::Cell, SearchBoard::kPaddedCellCount> m_cells{};

        std::array<std::uint8_t, kSlotCount> m_line_threat_slots{};
        std::array<std::uint8_t, kSlotCount> m_move_threat_slots{};

        std::array<Score, SearchBoard::kPaddedCellCount> m_x_move_scores{};
        std::array<Score, SearchBoard::kPaddedCellCount> m_o_move_scores{};
        std::array<Score, SearchBoard::kPaddedCellCount> m_x_tactical_move_scores{};
        std::array<Score, SearchBoard::kPaddedCellCount> m_o_tactical_move_scores{};

        std::array<Score, 2> m_global_score{};
        std::array<Score, 2> m_tactical_score_sum{};
    };

}
