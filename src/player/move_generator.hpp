#pragma once

#include <array>
#include <cstdint>

#include "core/game.hpp"
#include "engine_config.hpp"
#include "search_state.hpp"
#include "threat_types.hpp"

namespace ttt::my_player
{
    enum class MoveTier : std::uint8_t
    {
        WIN = 0,
        BLOCK_WIN,

        OPEN_FOUR,
        DOUBLE_FOUR,
        FOUR_THREE,

        BLOCK_OPEN_FOUR,
        DOUBLE_THREE,
        STRONG_DEFENSE,
        STRONG_ATTACK,
        QUIET,
    };

    struct ScoredMove
    {
        game::Point point{};
        std::uint16_t index = 0;

        MoveTier tier = MoveTier::QUIET;
        Score ordering_score = 0;

        MoveClass attack_class = MoveClass::QUIET;
    };

    struct MoveList
    {
        std::array<ScoredMove, SearchBoard::kBoardCellCount> moves{};
        int count = 0;

        void push(const ScoredMove &move) noexcept
        {
            if (count < static_cast<int>(moves.size()))
            {
                moves[count] = move;
                ++count;
            }
        }

        ScoredMove &operator[](int index) noexcept
        {
            return moves[index];
        }

        const ScoredMove &operator[](int index) const noexcept
        {
            return moves[index];
        }

        bool empty() const noexcept
        {
            return count == 0;
        }
    };

    class MoveGenerator
    {
    public:
        MoveList generate(
            const SearchState &state,
            game::Sign side,
            int max_quiet_moves) const;

    private:
        static constexpr int kCandidateRadius = EngineConfig::kCandidateRadius;

        static MoveTier classify_tier(
            MoveClass attack_class,
            MoveClass defense_class) noexcept;

        static Score tier_bonus(MoveTier tier) noexcept;

        static Score ordering_score(
            const IncrementalEvaluator &evaluator,
            std::uint16_t index,
            int x,
            int y,
            game::Sign side,
            game::Sign opponent,
            MoveTier tier) noexcept;

        static bool should_keep_after_prune(
            const ScoredMove &move,
            int &quiet_count,
            int max_quiet_moves) noexcept;

        static void sort_moves(MoveList &list) noexcept;
        static void prune_quiet_moves(MoveList &list, int max_quiet_moves) noexcept;
    };
}
