#pragma once

#include <array>
#include <cstdint>

namespace ttt::my_player
{

    using Score = std::int64_t;

    struct EngineConfig
    {
        static constexpr int kBoardPadding = 4;
        static constexpr int kBoardWidth = 20;
        static constexpr int kBoardHeight = 20;
        static constexpr int kWinLength = 5;

        static constexpr int kPatternWindowSize = 9;
        static constexpr int kPatternWindowCenter = 4;

        static constexpr int kMaxQuietMoves = 6;

        static constexpr int kSearchMaxDepth = 4;
        static constexpr int kSearchNodeLimit = 6'000;
        static constexpr int kSearchInnerQuietMoves = 6;
        static constexpr int kQuiescenceMaxDepth = 10;
        static constexpr Score kSearchInfinity = 4000000000000LL;
        static constexpr Score kSearchMateScore = 3000000000000LL;

        static constexpr int kCandidateRadius = 3;
        static constexpr int kCenterBonusBase = 100;

        static constexpr Score kMoveAttackMultiplier = 8LL;
        static constexpr Score kMoveDefenseMultiplier = 2LL;
        static constexpr Score kTacticalMoveAttackMultiplier = 1LL;
        static constexpr Score kTacticalMoveDefenseMultiplier = 2LL;
        static constexpr Score kTacticalLeafAttackMultiplier = 1LL;
        static constexpr Score kTacticalLeafDefenseMultiplier = 1LL;
        static constexpr Score kTacticalLeafDivisor = 4LL;

        static constexpr Score kWinTierBonus = 900000000000LL;
        static constexpr Score kBlockWinTierBonus = 850000000000LL;
        static constexpr Score kOpenFourTierBonus = 800000000000LL;
        static constexpr Score kDoubleFourTierBonus = 780000000000LL;
        static constexpr Score kFourThreeTierBonus = 760000000000LL;
        static constexpr Score kBlockOpenFourTierBonus = 720000000000LL;
        static constexpr Score kDoubleThreeTierBonus = 500000000000LL;
        static constexpr Score kStrongDefenseTierBonus = 20'000'000LL;
        static constexpr Score kStrongAttackTierBonus = 250'000'000LL;

        // Вес уже существующей угрозы в одном направлении
        inline static constexpr std::array<Score, 9> kDirectionThreatWeights = {
            0,         // NONE
            30,        // OPEN_TWO
            12,        // BROKEN_TWO
            180,       // SIMPLE_THREE
            2600,      // OPEN_THREE
            1200,      // BROKEN_THREE
            50000,     // SIMPLE_FOUR
            90000,     // OPEN_FOUR
            100000000, // FIVE
        };

        // Вес потенциального хода по его классу
        inline static constexpr std::array<Score, 12> kMoveClassWeights = {
            0,         // QUIET
            40,        // OPEN_TWO
            16,        // BROKEN_TWO
            350,       // SIMPLE_THREE
            2200,      // OPEN_THREE
            1400,      // BROKEN_THREE
            65000,     // SIMPLE_FOUR
            500000,    // OPEN_FOUR
            350000,    // DOUBLE_THREE
            700000,    // FOUR_THREE
            900000,    // DOUBLE_FOUR
            100000000, // WIN
        };
    };

}
