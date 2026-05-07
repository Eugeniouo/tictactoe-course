#pragma once

#include <array>
#include <cstdint>

#include "engine_config.hpp"
#include "threat_types.hpp"

namespace ttt::my_player
{

    struct PatternEntry
    {
        std::uint8_t line_threats = 0;
        std::uint8_t move_threats = 0;
    };

    namespace pattern_detail
    {

        inline constexpr int kWindowSize = EngineConfig::kPatternWindowSize;
        inline constexpr int kWindowCenter = EngineConfig::kPatternWindowCenter;
        inline constexpr int kPatternCodeCount = 1 << (2 * kWindowSize);

        static_assert(kWindowCenter * 2 + 1 == kWindowSize);

    }

    extern const std::array<PatternEntry, pattern_detail::kPatternCodeCount>
        kPatternTable;

}
