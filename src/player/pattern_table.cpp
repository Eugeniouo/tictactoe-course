/**
 * @file pattern_table.cpp
 * @brief Построение таблицы угроз для всех возможных линейных окон.
 *
 * Файл перебирает все коды окна длиной 9 клеток и определяет,
 * какие угрозы есть у X и O в линии и какие угрозы возникнут после хода в центр окна. 
 * Проверки пятерок, четверок, троек и двоек.
 *
 * Таблица нужна, чтобы IncrementalEvaluator не анализировал комбинации
 * вручную при каждом ходе. Быстрый доступ по коду окна ускоряет оценку позиции
 * и делает классификацию угроз единообразной во всем движке.
 */

#include "pattern_table.hpp"

namespace ttt::my_player
{

    namespace
    {

        constexpr std::uint8_t kEmpty = 0;
        constexpr std::uint8_t kX = 1;
        constexpr std::uint8_t kO = 2;
        constexpr std::uint8_t kWall = 3;

        std::uint8_t opponent_of(std::uint8_t side) noexcept
        {
            return side == kX ? kO : kX;
        }

        std::uint8_t cell_at(std::uint32_t code, int position) noexcept
        {
            return static_cast<std::uint8_t>((code >> (2 * position)) & 0b11U);
        }

        std::uint32_t set_cell(
            std::uint32_t code,
            int position,
            std::uint8_t cell) noexcept
        {
            const std::uint32_t mask = 0b11U << (2 * position);

            code &= ~mask;
            code |= static_cast<std::uint32_t>(cell) << (2 * position);

            return code;
        }

        bool is_empty(std::uint8_t cell) noexcept
        {
            return cell == kEmpty;
        }

        bool is_blocker(std::uint8_t cell, std::uint8_t side) noexcept
        {
            return cell == kWall || cell == opponent_of(side);
        }

        bool has_same_side_left_before_blocker(
            std::uint32_t code,
            std::uint8_t side) noexcept
        {
            for (int position = pattern_detail::kWindowCenter - 1;
                 position >= 0;
                 --position)
            {
                const std::uint8_t cell = cell_at(code, position);

                if (is_blocker(cell, side))
                {
                    return false;
                }

                if (cell == side)
                {
                    return true;
                }
            }

            return false;
        }

        bool has_five_including_position(
            std::uint32_t code,
            std::uint8_t side,
            int required_position) noexcept
        {
            for (int start = 0;
                 start <= pattern_detail::kWindowSize - 5;
                 ++start)
            {
                if (required_position < start || required_position >= start + 5)
                {
                    continue;
                }

                bool five = true;

                for (int offset = 0; offset < 5; ++offset)
                {
                    if (cell_at(code, start + offset) != side)
                    {
                        five = false;
                        break;
                    }
                }

                if (five)
                {
                    return true;
                }
            }

            return false;
        }

        int count_winning_extensions_including_position(
            std::uint32_t code,
            std::uint8_t side,
            int required_position) noexcept
        {
            int count = 0;

            for (int position = 0;
                 position < pattern_detail::kWindowSize;
                 ++position)
            {
                if (!is_empty(cell_at(code, position)))
                {
                    continue;
                }

                const std::uint32_t next_code = set_cell(code, position, side);

                if (has_five_including_position(
                        next_code,
                        side,
                        required_position))
                {
                    ++count;
                }
            }

            return count;
        }

        DirectionThreat classify_four_level(
            std::uint32_t code,
            std::uint8_t side,
            int required_position) noexcept
        {
            if (has_five_including_position(code, side, required_position))
            {
                return DirectionThreat::FIVE;
            }

            const int winning_extensions =
                count_winning_extensions_including_position(
                    code,
                    side,
                    required_position);

            if (winning_extensions >= 2)
            {
                return DirectionThreat::OPEN_FOUR;
            }

            if (winning_extensions == 1)
            {
                return DirectionThreat::SIMPLE_FOUR;
            }

            return DirectionThreat::NONE;
        }

        DirectionThreat classify_three_level(
            std::uint32_t code,
            std::uint8_t side,
            int required_position) noexcept
        {
            int open_four_extensions = 0;
            int simple_four_extensions = 0;

            for (int position = 0;
                 position < pattern_detail::kWindowSize;
                 ++position)
            {
                if (!is_empty(cell_at(code, position)))
                {
                    continue;
                }

                const std::uint32_t next_code = set_cell(code, position, side);
                const DirectionThreat threat =
                    classify_four_level(next_code, side, required_position);

                if (threat == DirectionThreat::FIVE ||
                    threat == DirectionThreat::OPEN_FOUR)
                {
                    ++open_four_extensions;
                }
                else if (threat == DirectionThreat::SIMPLE_FOUR)
                {
                    ++simple_four_extensions;
                }
            }

            if (open_four_extensions >= 2)
            {
                return DirectionThreat::OPEN_THREE;
            }

            if (open_four_extensions == 1)
            {
                return DirectionThreat::BROKEN_THREE;
            }

            if (simple_four_extensions >= 2)
            {
                return DirectionThreat::BROKEN_THREE;
            }

            if (simple_four_extensions == 1)
            {
                return DirectionThreat::SIMPLE_THREE;
            }

            return DirectionThreat::NONE;
        }

        DirectionThreat classify_two_level(
            std::uint32_t code,
            std::uint8_t side,
            int required_position) noexcept
        {
            int open_three_extensions = 0;
            int other_three_extensions = 0;

            for (int position = 0;
                 position < pattern_detail::kWindowSize;
                 ++position)
            {
                if (!is_empty(cell_at(code, position)))
                {
                    continue;
                }

                const std::uint32_t next_code = set_cell(code, position, side);
                const DirectionThreat threat =
                    classify_three_level(next_code, side, required_position);

                if (threat == DirectionThreat::OPEN_THREE)
                {
                    ++open_three_extensions;
                }
                else if (threat == DirectionThreat::BROKEN_THREE ||
                         threat == DirectionThreat::SIMPLE_THREE)
                {
                    ++other_three_extensions;
                }
            }

            if (open_three_extensions >= 2)
            {
                return DirectionThreat::OPEN_TWO;
            }

            if (open_three_extensions == 1 || other_three_extensions > 0)
            {
                return DirectionThreat::BROKEN_TWO;
            }

            return DirectionThreat::NONE;
        }

        DirectionThreat classify_existing_threat(
            std::uint32_t code,
            std::uint8_t side,
            int required_position) noexcept
        {
            const DirectionThreat four_level =
                classify_four_level(code, side, required_position);

            if (four_level != DirectionThreat::NONE)
            {
                return four_level;
            }

            const DirectionThreat three_level =
                classify_three_level(code, side, required_position);

            if (three_level != DirectionThreat::NONE)
            {
                return three_level;
            }

            return classify_two_level(code, side, required_position);
        }

        DirectionThreat classify_line_threat(
            std::uint32_t code,
            std::uint8_t side) noexcept
        {
            if (cell_at(code, pattern_detail::kWindowCenter) != side)
            {
                return DirectionThreat::NONE;
            }

            if (has_same_side_left_before_blocker(code, side))
            {
                return DirectionThreat::NONE;
            }

            return classify_existing_threat(
                code,
                side,
                pattern_detail::kWindowCenter);
        }

        DirectionThreat classify_move_threat(
            std::uint32_t code,
            std::uint8_t side) noexcept
        {
            if (cell_at(code, pattern_detail::kWindowCenter) != kEmpty)
            {
                return DirectionThreat::NONE;
            }

            const std::uint32_t code_after_move =
                set_cell(code, pattern_detail::kWindowCenter, side);

            return classify_existing_threat(
                code_after_move,
                side,
                pattern_detail::kWindowCenter);
        }

        PatternEntry make_pattern_entry(std::uint32_t code) noexcept
        {
            const DirectionThreat line_x = classify_line_threat(code, kX);
            const DirectionThreat line_o = classify_line_threat(code, kO);

            const DirectionThreat move_x = classify_move_threat(code, kX);
            const DirectionThreat move_o = classify_move_threat(code, kO);

            return PatternEntry{
                pack_threats(line_x, line_o),
                pack_threats(move_x, move_o),
            };
        }

        std::array<PatternEntry, pattern_detail::kPatternCodeCount>
        make_pattern_table()
        {
            std::array<PatternEntry, pattern_detail::kPatternCodeCount> table{};

            for (int code = 0; code < pattern_detail::kPatternCodeCount; ++code)
            {
                table[code] = make_pattern_entry(static_cast<std::uint32_t>(code));
            }

            return table;
        }

    }

    const std::array<PatternEntry, pattern_detail::kPatternCodeCount>
        kPatternTable = make_pattern_table();

}
