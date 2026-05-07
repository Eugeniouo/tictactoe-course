/**
 * @file move_combination_table.hpp
 * @brief Классификация хода по угрозам в четырех направлениях.
 *
 * После того как PatternTable определил угрозу отдельно для горизонтали,
 * вертикали и двух диагоналей, этот файл объединяет четыре значения в общий
 * MoveClass.
 *
 * Файл нужен для сборки картины вокруг одной клетки.
 * одиночные угрозы превращаются в ход, который
 * получает вес в оценщике и место в очереди перебора.
 */

#pragma once

#include <cstdint>

#include "threat_types.hpp"

namespace ttt::my_player
{

    namespace move_combination_detail
    {

        constexpr int combination_code(
            DirectionThreat a,
            DirectionThreat b,
            DirectionThreat c,
            DirectionThreat d) noexcept
        {
            return static_cast<int>(a) |
                   (static_cast<int>(b) << 4) |
                   (static_cast<int>(c) << 8) |
                   (static_cast<int>(d) << 12);
        }

        constexpr DirectionThreat threat_from_code(int code, int position) noexcept
        {
            return static_cast<DirectionThreat>((code >> (position * 4)) & 0x0F);
        }

        constexpr MoveClass classify_combination(int code) noexcept
        {
            int five_count = 0;
            int open_four_count = 0;
            int simple_four_count = 0;
            int open_three_count = 0;
            int broken_three_count = 0;
            int simple_three_count = 0;
            int open_two_count = 0;
            int broken_two_count = 0;

            for (int i = 0; i < 4; ++i)
            {
                const DirectionThreat threat = threat_from_code(code, i);

                switch (threat)
                {
                case DirectionThreat::FIVE:
                    ++five_count;
                    break;

                case DirectionThreat::OPEN_FOUR:
                    ++open_four_count;
                    break;

                case DirectionThreat::SIMPLE_FOUR:
                    ++simple_four_count;
                    break;

                case DirectionThreat::OPEN_THREE:
                    ++open_three_count;
                    break;

                case DirectionThreat::BROKEN_THREE:
                    ++broken_three_count;
                    break;

                case DirectionThreat::SIMPLE_THREE:
                    ++simple_three_count;
                    break;

                case DirectionThreat::OPEN_TWO:
                    ++open_two_count;
                    break;

                case DirectionThreat::BROKEN_TWO:
                    ++broken_two_count;
                    break;

                case DirectionThreat::NONE:
                case DirectionThreat::COUNT:
                default:
                    break;
                }
            }

            if (five_count > 0)
            {
                return MoveClass::WIN;
            }

            if (open_four_count > 0)
            {
                return MoveClass::OPEN_FOUR;
            }

            if (simple_four_count >= 2)
            {
                return MoveClass::DOUBLE_FOUR;
            }

            if (simple_four_count >= 1 &&
                open_three_count + broken_three_count + simple_three_count >= 1)
            {
                return MoveClass::FOUR_THREE;
            }

            if (open_three_count >= 2)
            {
                return MoveClass::DOUBLE_THREE;
            }

            if (open_three_count >= 1 && broken_three_count >= 1)
            {
                return MoveClass::DOUBLE_THREE;
            }

            if (simple_four_count == 1)
            {
                return MoveClass::SIMPLE_FOUR;
            }

            if (open_three_count == 1)
            {
                return MoveClass::OPEN_THREE;
            }

            if (broken_three_count > 0)
            {
                return MoveClass::BROKEN_THREE;
            }

            if (simple_three_count > 0)
            {
                return MoveClass::SIMPLE_THREE;
            }

            if (open_two_count > 0)
            {
                return MoveClass::OPEN_TWO;
            }

            if (broken_two_count > 0)
            {
                return MoveClass::BROKEN_TWO;
            }

            return MoveClass::QUIET;
        }

    }

    inline MoveClass move_class_from_direction_threats(
        DirectionThreat a,
        DirectionThreat b,
        DirectionThreat c,
        DirectionThreat d) noexcept
    {
        const int code =
            move_combination_detail::combination_code(a, b, c, d);

        return move_combination_detail::classify_combination(code);
    }

}
