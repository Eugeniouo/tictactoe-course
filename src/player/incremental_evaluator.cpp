/**
 * @file incremental_evaluator.cpp
 * @brief Реализация быстрого пересчета угроз и оценок после изменения позиции
 *
 * Файл кодирует девятиклеточные окна, берет их классы из kPatternTable,
 * поддерживает глобальные суммы угроз для X и O, также отдельные
 * оценки потенциальных ходов. При ходе или откате пересчитываются только линии,
 * которые могли измениться.
 *
 * Он сообщает поиску, какая сторона лучше стоит в листе, а 
 * генератору ходов какие клетки создают или закрывают
 * опасные комбинации.
 */

#include "incremental_evaluator.hpp"

#include <cassert>

#include "move_combination_table.hpp"
#include "pattern_table.hpp"

namespace ttt::my_player
{

    void IncrementalEvaluator::load_from_board(const SearchBoard &board)
    {
        m_cells.fill(SearchBoard::Cell::WALL);

        m_line_threat_slots.fill(0);
        m_move_threat_slots.fill(0);

        m_x_move_scores.fill(0);
        m_o_move_scores.fill(0);
        m_x_tactical_move_scores.fill(0);
        m_o_tactical_move_scores.fill(0);

        m_global_score[0] = 0;
        m_global_score[1] = 0;
        m_tactical_score_sum[0] = 0;
        m_tactical_score_sum[1] = 0;

        for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
        {
            for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
            {
                const int index = SearchBoard::to_index(x, y);
                m_cells[index] = board.get_cell(x, y);
            }
        }

        for (int index = 0; index < SearchBoard::kPaddedCellCount; ++index)
        {
            if (!is_playable_index(index))
            {
                continue;
            }

            for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
            {
                recompute_directed_slot(index, direction_id);
                add_line_slot(slot_index(index, direction_id));
            }
        }

        for (int index = 0; index < SearchBoard::kPaddedCellCount; ++index)
        {
            if (is_playable_index(index))
            {
                recompute_move_cell(index);
            }
        }
    }

    void IncrementalEvaluator::apply_move(
        std::uint16_t index,
        SearchBoard::Cell cell) noexcept
    {
        assert(is_playable_index(index));
        assert(is_stone_cell(cell));
        assert(m_cells[index] == SearchBoard::Cell::EMPTY);

        std::array<std::uint16_t, 33> affected_cells{};
        const int affected_cell_count =
            collect_affected_cells(index, affected_cells);

        for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
        {
            const int step = kDirections[direction_id];

            for (int offset = -4; offset <= 4; ++offset)
            {
                const int center = index + offset * step;

                if (!is_playable_index(center))
                {
                    continue;
                }

                remove_line_slot(slot_index(center, direction_id));
            }
        }

        m_cells[index] = cell;

        for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
        {
            const int step = kDirections[direction_id];

            for (int offset = -4; offset <= 4; ++offset)
            {
                const int center = index + offset * step;

                if (!is_playable_index(center))
                {
                    continue;
                }

                recompute_directed_slot(center, direction_id);
                add_line_slot(slot_index(center, direction_id));
            }
        }

        for (int i = 0; i < affected_cell_count; ++i)
        {
            recompute_move_cell(affected_cells[i]);
        }
    }

    void IncrementalEvaluator::undo_move(std::uint16_t index) noexcept
    {
        assert(is_playable_index(index));
        assert(is_stone_cell(m_cells[index]));

        std::array<std::uint16_t, 33> affected_cells{};
        const int affected_cell_count =
            collect_affected_cells(index, affected_cells);

        for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
        {
            const int step = kDirections[direction_id];

            for (int offset = -4; offset <= 4; ++offset)
            {
                const int center = index + offset * step;

                if (!is_playable_index(center))
                {
                    continue;
                }

                remove_line_slot(slot_index(center, direction_id));
            }
        }

        m_cells[index] = SearchBoard::Cell::EMPTY;

        for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
        {
            const int step = kDirections[direction_id];

            for (int offset = -4; offset <= 4; ++offset)
            {
                const int center = index + offset * step;

                if (!is_playable_index(center))
                {
                    continue;
                }

                recompute_directed_slot(center, direction_id);
                add_line_slot(slot_index(center, direction_id));
            }
        }

        for (int i = 0; i < affected_cell_count; ++i)
        {
            recompute_move_cell(affected_cells[i]);
        }
    }

    Score IncrementalEvaluator::evaluate(game::Sign side) const noexcept
    {
        const int me = side_index(side);
        const int opponent = 1 - me;

        return m_global_score[me] - m_global_score[opponent] +
               (m_tactical_score_sum[me] *
                    EngineConfig::kTacticalLeafAttackMultiplier -
                m_tactical_score_sum[opponent] *
                    EngineConfig::kTacticalLeafDefenseMultiplier) /
                   EngineConfig::kTacticalLeafDivisor;
    }

    Score IncrementalEvaluator::move_score(
        std::uint16_t index,
        game::Sign side) const noexcept
    {
        const int side_id = side_index(side);

        if (side_id == 0)
        {
            return m_x_move_scores[index];
        }

        return m_o_move_scores[index];
    }

    Score IncrementalEvaluator::tactical_move_score(
        std::uint16_t index,
        game::Sign side) const noexcept
    {
        const int side_id = side_index(side);

        if (side_id == 0)
        {
            return m_x_tactical_move_scores[index];
        }

        return m_o_tactical_move_scores[index];
    }

    MoveClass IncrementalEvaluator::move_class(
        std::uint16_t index,
        game::Sign side) const noexcept
    {
        if (!is_playable_index(index))
        {
            return MoveClass::QUIET;
        }

        if (m_cells[index] != SearchBoard::Cell::EMPTY)
        {
            return MoveClass::QUIET;
        }

        const int side_id = side_index(side);

        const DirectionThreat t0 =
            unpack_side_threat(m_move_threat_slots[slot_index(index, 0)], side_id);

        const DirectionThreat t1 =
            unpack_side_threat(m_move_threat_slots[slot_index(index, 1)], side_id);

        const DirectionThreat t2 =
            unpack_side_threat(m_move_threat_slots[slot_index(index, 2)], side_id);

        const DirectionThreat t3 =
            unpack_side_threat(m_move_threat_slots[slot_index(index, 3)], side_id);

        return move_class_from_direction_threats(t0, t1, t2, t3);
    }

    int IncrementalEvaluator::side_index(game::Sign side) noexcept
    {
        switch (side)
        {
        case game::Sign::X:
            return 0;

        case game::Sign::O:
            return 1;

        default:
            assert(false);
            return 0;
        }
    }

    bool IncrementalEvaluator::is_playable_index(int index) noexcept
    {
        if (index < 0 || index >= SearchBoard::kPaddedCellCount)
        {
            return false;
        }

        const int x = index % SearchBoard::kPaddedWidth;
        const int y = index / SearchBoard::kPaddedWidth;

        return x >= SearchBoard::kPadding &&
               x < SearchBoard::kPadding + SearchBoard::kBoardWidth &&
               y >= SearchBoard::kPadding &&
               y < SearchBoard::kPadding + SearchBoard::kBoardHeight;
    }

    DirectionThreat IncrementalEvaluator::unpack_side_threat(
        std::uint8_t packed,
        int side) noexcept
    {
        return side == 0 ? unpack_x_threat(packed) : unpack_o_threat(packed);
    }

    SearchBoard::Cell IncrementalEvaluator::cell_for_side(
        game::Sign side) noexcept
    {
        return side == game::Sign::X
                   ? SearchBoard::Cell::X
                   : SearchBoard::Cell::O;
    }

    int IncrementalEvaluator::tactical_line_value(
        SearchBoard::Cell cell,
        SearchBoard::Cell side_cell) noexcept
    {
        if (cell == side_cell)
        {
            return 1;
        }

        if (cell == SearchBoard::Cell::EMPTY)
        {
            return 0;
        }

        return 2;
    }

    Score IncrementalEvaluator::tactical_window_score(
        const int line[9],
        int start) noexcept
    {
        int stones = 0;
        int blocked = 0;

        for (int i = 0; i < 5; ++i)
        {
            const int value = line[start + i];

            if (value == 1)
            {
                ++stones;
            }
            else if (value == 2)
            {
                ++blocked;
            }
        }

        if (blocked > 0)
        {
            return 0;
        }

        if (stones == 5)
        {
            return 100000000;
        }

        if (stones == 4)
        {
            return 20000;
        }

        if (stones == 3)
        {
            return line[start] == 0 && line[start + 4] == 0
                       ? 100000
                       : 5000;
        }

        if (stones == 2)
        {
            return line[start] == 0 && line[start + 4] == 0
                       ? 10000
                       : 1000;
        }

        return 0;
    }

    Score IncrementalEvaluator::tactical_line_score(
        const int line[9]) noexcept
    {
        Score score = 0;

        for (int start = 0; start <= 4; ++start)
        {
            score += tactical_window_score(line, start);
        }

        for (int start = 0; start <= 3; ++start)
        {
            if (line[start] == 0 &&
                line[start + 1] == 1 &&
                line[start + 2] == 1 &&
                line[start + 3] == 1 &&
                line[start + 4] == 1 &&
                line[start + 5] == 0)
            {
                score += 1000000;
            }
        }

        return score;
    }

    std::uint32_t IncrementalEvaluator::encode_window(
        int index,
        int direction_id) const noexcept
    {
        assert(is_playable_index(index));
        assert(direction_id >= 0);
        assert(direction_id < kDirectionCount);

        const int step = kDirections[direction_id];

        int current = index - 4 * step;
        std::uint32_t code = 0;

        for (int position = 0; position < 9; ++position)
        {
            const auto cell =
                static_cast<std::uint32_t>(m_cells[current]);

            code |= cell << (2 * position);

            current += step;
        }

        return code;
    }

    Score IncrementalEvaluator::compute_tactical_move_score(
        int index,
        SearchBoard::Cell side_cell) const noexcept
    {
        if (!is_playable_index(index) ||
            m_cells[index] != SearchBoard::Cell::EMPTY)
        {
            return 0;
        }

        Score score = 0;
        int threat_count = 0;

        for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
        {
            const int step = kDirections[direction_id];
            int line[9]{};

            for (int offset = -4; offset <= 4; ++offset)
            {
                const int position = offset + 4;

                if (offset == 0)
                {
                    line[position] = 1;
                    continue;
                }

                line[position] =
                    tactical_line_value(
                        m_cells[index + offset * step],
                        side_cell);
            }

            const Score direction_score = tactical_line_score(line);

            if (direction_score >= 1000)
            {
                ++threat_count;
            }

            score += direction_score;
        }

        if (threat_count >= 2)
        {
            score *= 10;
        }

        return score;
    }

    void IncrementalEvaluator::recompute_directed_slot(
        int index,
        int direction_id) noexcept
    {
        assert(is_playable_index(index));

        const int slot = slot_index(index, direction_id);
        const std::uint32_t code = encode_window(index, direction_id);

        const PatternEntry entry = kPatternTable[code];

        m_line_threat_slots[slot] = entry.line_threats;
        m_move_threat_slots[slot] = entry.move_threats;
    }

    void IncrementalEvaluator::remove_line_slot(int slot) noexcept
    {
        const std::uint8_t packed = m_line_threat_slots[slot];

        const DirectionThreat x_threat = unpack_x_threat(packed);
        const DirectionThreat o_threat = unpack_o_threat(packed);

        m_global_score[0] -= direction_threat_weight(x_threat);
        m_global_score[1] -= direction_threat_weight(o_threat);
    }

    void IncrementalEvaluator::add_line_slot(int slot) noexcept
    {
        const std::uint8_t packed = m_line_threat_slots[slot];

        const DirectionThreat x_threat = unpack_x_threat(packed);
        const DirectionThreat o_threat = unpack_o_threat(packed);

        m_global_score[0] += direction_threat_weight(x_threat);
        m_global_score[1] += direction_threat_weight(o_threat);
    }

    void IncrementalEvaluator::recompute_move_cell(int index) noexcept
    {
        assert(is_playable_index(index));

        m_tactical_score_sum[0] -= m_x_tactical_move_scores[index];
        m_tactical_score_sum[1] -= m_o_tactical_move_scores[index];

        m_x_move_scores[index] = 0;
        m_o_move_scores[index] = 0;
        m_x_tactical_move_scores[index] = 0;
        m_o_tactical_move_scores[index] = 0;

        if (m_cells[index] != SearchBoard::Cell::EMPTY)
        {
            return;
        }

        {
            const MoveClass move_class_for_x =
                move_class(static_cast<std::uint16_t>(index), game::Sign::X);

            m_x_move_scores[index] = move_class_weight(move_class_for_x);
            m_x_tactical_move_scores[index] =
                compute_tactical_move_score(
                    index,
                    cell_for_side(game::Sign::X));
        }

        {
            const MoveClass move_class_for_o =
                move_class(static_cast<std::uint16_t>(index), game::Sign::O);

            m_o_move_scores[index] = move_class_weight(move_class_for_o);
            m_o_tactical_move_scores[index] =
                compute_tactical_move_score(
                    index,
                    cell_for_side(game::Sign::O));
        }

        m_tactical_score_sum[0] += m_x_tactical_move_scores[index];
        m_tactical_score_sum[1] += m_o_tactical_move_scores[index];
    }

    int IncrementalEvaluator::collect_affected_cells(
        int index,
        std::array<std::uint16_t, 33> &out) const noexcept
    {
        int count = 0;

        for (int direction_id = 0; direction_id < kDirectionCount; ++direction_id)
        {
            const int step = kDirections[direction_id];

            for (int offset = -4; offset <= 4; ++offset)
            {
                const int affected = index + offset * step;

                if (!is_playable_index(affected))
                {
                    continue;
                }

                const auto affected_u16 =
                    static_cast<std::uint16_t>(affected);

                if (contains_cell(out, count, affected_u16))
                {
                    continue;
                }

                assert(count < static_cast<int>(out.size()));

                out[count] = affected_u16;
                ++count;
            }
        }

        return count;
    }

    bool IncrementalEvaluator::contains_cell(
        const std::array<std::uint16_t, 33> &cells,
        int count,
        std::uint16_t value) noexcept
    {
        for (int i = 0; i < count; ++i)
        {
            if (cells[i] == value)
            {
                return true;
            }
        }

        return false;
    }

}
