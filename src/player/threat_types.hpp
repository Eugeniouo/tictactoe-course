/**
 * @file threat_types.hpp
 * @brief Типы угроз и классов ходов, используемые оценкой и генератором.
 *
 * Файл задает перечисления DirectionThreat` и `MoveClass`, функции получения
 * весов из `EngineConfig`, а также упаковку угроз X и O в один байт. Эти типы
 * являются общим языком между таблицей шаблонов, оценщиком и генератором ходов.
 *
 */

#pragma once

#include <cstdint>

#include "engine_config.hpp"

namespace ttt::my_player
{

    enum class DirectionThreat : std::uint8_t
    {
        NONE = 0,

        OPEN_TWO,
        BROKEN_TWO,

        SIMPLE_THREE,
        OPEN_THREE,
        BROKEN_THREE,

        SIMPLE_FOUR,
        OPEN_FOUR,

        FIVE,

        COUNT
    };

    enum class MoveClass : std::uint8_t
    {
        QUIET = 0,

        OPEN_TWO,
        BROKEN_TWO,

        SIMPLE_THREE,
        OPEN_THREE,
        BROKEN_THREE,

        SIMPLE_FOUR,
        OPEN_FOUR,

        DOUBLE_THREE,
        FOUR_THREE,
        DOUBLE_FOUR,

        WIN,

        COUNT
    };

    constexpr int direction_threat_index(DirectionThreat threat) noexcept
    {
        return static_cast<int>(threat);
    }

    constexpr int move_class_index(MoveClass move_class) noexcept
    {
        return static_cast<int>(move_class);
    }

    static_assert(direction_threat_index(DirectionThreat::COUNT) ==
                  static_cast<int>(EngineConfig::kDirectionThreatWeights.size()));
    static_assert(move_class_index(MoveClass::COUNT) ==
                  static_cast<int>(EngineConfig::kMoveClassWeights.size()));

    constexpr Score direction_threat_weight(DirectionThreat threat) noexcept
    {
        return EngineConfig::kDirectionThreatWeights[direction_threat_index(threat)];
    }

    constexpr Score move_class_weight(MoveClass move_class) noexcept
    {
        return EngineConfig::kMoveClassWeights[move_class_index(move_class)];
    }

    constexpr std::uint8_t pack_threats(
        DirectionThreat x_threat,
        DirectionThreat o_threat) noexcept
    {
        return static_cast<std::uint8_t>(x_threat) |
               static_cast<std::uint8_t>(
                   static_cast<std::uint8_t>(o_threat) << 4);
    }

    constexpr DirectionThreat unpack_x_threat(std::uint8_t packed) noexcept
    {
        return static_cast<DirectionThreat>(packed & 0x0F);
    }

    constexpr DirectionThreat unpack_o_threat(std::uint8_t packed) noexcept
    {
        return static_cast<DirectionThreat>((packed >> 4) & 0x0F);
    }

}
