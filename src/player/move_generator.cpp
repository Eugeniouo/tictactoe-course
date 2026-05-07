#include "move_generator.hpp"

#include <algorithm>
#include <array>

#include "board_utils.hpp"

namespace ttt::my_player
{

    MoveList MoveGenerator::generate(
        const SearchState &state,
        game::Sign side,
        int max_quiet_moves) const
    {
        MoveList list;

        const SearchBoard &board = state.board();
        const IncrementalEvaluator &evaluator = state.evaluator();

        const game::Sign opponent = SearchBoard::opposite_player_sign(side);

        std::array<bool, SearchBoard::kPaddedCellCount> used{};

        if (board.stone_count() == 0)
        {
            int best_x = -1;
            int best_y = -1;
            int best_distance =
                SearchBoard::kBoardWidth + SearchBoard::kBoardHeight + 1;

            for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
            {
                for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
                {
                    if (!is_empty_cell(board, x, y))
                    {
                        continue;
                    }

                    const int distance = center_distance(x, y);

                    if (distance < best_distance)
                    {
                        best_distance = distance;
                        best_x = x;
                        best_y = y;
                    }
                }
            }

            if (best_x >= 0)
            {
                ScoredMove move;
                move.point = make_point(best_x, best_y);
                move.index = static_cast<std::uint16_t>(
                    SearchBoard::to_index(best_x, best_y));
                move.tier = MoveTier::QUIET;
                move.ordering_score = center_bonus(best_x, best_y);

                list.push(move);
            }

            return list;
        }

        for (int stone_pos = 0; stone_pos < board.stone_count(); ++stone_pos)
        {
            const std::uint16_t stone_index = board.stone_index_at(stone_pos);

            const int stone_x = board_x_from_index(stone_index);
            const int stone_y = board_y_from_index(stone_index);

            for (int dy = -kCandidateRadius; dy <= kCandidateRadius; ++dy)
            {
                for (int dx = -kCandidateRadius; dx <= kCandidateRadius; ++dx)
                {
                    if (dx == 0 && dy == 0)
                    {
                        continue;
                    }

                    const int x = stone_x + dx;
                    const int y = stone_y + dy;

                    if (!is_empty_cell(board, x, y))
                    {
                        continue;
                    }

                    const int index = SearchBoard::to_index(x, y);

                    if (used[index])
                    {
                        continue;
                    }

                    used[index] = true;

                    const auto index_u16 = static_cast<std::uint16_t>(index);

                    const MoveClass attack_class =
                        evaluator.move_class(index_u16, side);

                    const MoveClass defense_class =
                        evaluator.move_class(index_u16, opponent);

                    const MoveTier tier =
                        classify_tier(attack_class, defense_class);

                    ScoredMove move;
                    move.point = make_point(x, y);
                    move.index = index_u16;
                    move.tier = tier;
                    move.attack_class = attack_class;
                    move.ordering_score =
                        ordering_score(
                            evaluator,
                            index_u16,
                            x,
                            y,
                            side,
                            opponent,
                            tier);

                    list.push(move);
                }
            }
        }

        sort_moves(list);
        prune_quiet_moves(list, max_quiet_moves);

        return list;
    }

    MoveTier MoveGenerator::classify_tier(
        MoveClass attack_class,
        MoveClass defense_class) noexcept
    {
        if (attack_class == MoveClass::WIN)
        {
            return MoveTier::WIN;
        }

        if (defense_class == MoveClass::WIN)
        {
            return MoveTier::BLOCK_WIN;
        }

        if (attack_class == MoveClass::OPEN_FOUR)
        {
            return MoveTier::OPEN_FOUR;
        }

        if (attack_class == MoveClass::DOUBLE_FOUR)
        {
            return MoveTier::DOUBLE_FOUR;
        }

        if (attack_class == MoveClass::FOUR_THREE)
        {
            return MoveTier::FOUR_THREE;
        }

        if (defense_class == MoveClass::OPEN_FOUR)
        {
            return MoveTier::BLOCK_OPEN_FOUR;
        }

        if (attack_class == MoveClass::DOUBLE_THREE)
        {
            return MoveTier::DOUBLE_THREE;
        }

        if (defense_class == MoveClass::DOUBLE_FOUR ||
            defense_class == MoveClass::FOUR_THREE ||
            defense_class == MoveClass::DOUBLE_THREE ||
            defense_class == MoveClass::SIMPLE_FOUR ||
            defense_class == MoveClass::OPEN_THREE)
        {
            return MoveTier::STRONG_DEFENSE;
        }

        if (attack_class == MoveClass::SIMPLE_FOUR ||
            attack_class == MoveClass::OPEN_THREE ||
            attack_class == MoveClass::BROKEN_THREE)
        {
            return MoveTier::STRONG_ATTACK;
        }

        return MoveTier::QUIET;
    }

    Score MoveGenerator::tier_bonus(MoveTier tier) noexcept
    {
        switch (tier)
        {
        case MoveTier::WIN:
            return EngineConfig::kWinTierBonus;

        case MoveTier::BLOCK_WIN:
            return EngineConfig::kBlockWinTierBonus;

        case MoveTier::OPEN_FOUR:
            return EngineConfig::kOpenFourTierBonus;

        case MoveTier::DOUBLE_FOUR:
            return EngineConfig::kDoubleFourTierBonus;

        case MoveTier::FOUR_THREE:
            return EngineConfig::kFourThreeTierBonus;

        case MoveTier::BLOCK_OPEN_FOUR:
            return EngineConfig::kBlockOpenFourTierBonus;

        case MoveTier::DOUBLE_THREE:
            return EngineConfig::kDoubleThreeTierBonus;

        case MoveTier::STRONG_DEFENSE:
            return EngineConfig::kStrongDefenseTierBonus;

        case MoveTier::STRONG_ATTACK:
            return EngineConfig::kStrongAttackTierBonus;

        case MoveTier::QUIET:
        default:
            return 0;
        }
    }

    Score MoveGenerator::ordering_score(
        const IncrementalEvaluator &evaluator,
        std::uint16_t index,
        int x,
        int y,
        game::Sign side,
        game::Sign opponent,
        MoveTier tier) noexcept
    {
        const Score attack = evaluator.move_score(index, side);
        const Score defense = evaluator.move_score(index, opponent);
        const Score tactical_attack =
            evaluator.tactical_move_score(index, side);
        const Score tactical_defense =
            evaluator.tactical_move_score(index, opponent);

        return tier_bonus(tier) +
               attack * EngineConfig::kMoveAttackMultiplier +
               defense * EngineConfig::kMoveDefenseMultiplier +
               tactical_attack *
                   EngineConfig::kTacticalMoveAttackMultiplier +
               tactical_defense *
                   EngineConfig::kTacticalMoveDefenseMultiplier +
               center_bonus(x, y);
    }

    void MoveGenerator::sort_moves(MoveList &list) noexcept
    {
        std::sort(
            list.moves.begin(),
            list.moves.begin() + list.count,
            [](const ScoredMove &lhs, const ScoredMove &rhs)
            {
                if (lhs.tier != rhs.tier)
                {
                    return static_cast<int>(lhs.tier) <
                           static_cast<int>(rhs.tier);
                }

                return lhs.ordering_score > rhs.ordering_score;
            });
    }

    bool MoveGenerator::should_keep_after_prune(
        const ScoredMove &move,
        int &quiet_count,
        int max_quiet_moves) noexcept
    {
        if (move.tier != MoveTier::QUIET)
        {
            return true;
        }

        if (quiet_count < max_quiet_moves)
        {
            ++quiet_count;
            return true;
        }

        return false;
    }

    void MoveGenerator::prune_quiet_moves(
        MoveList &list,
        int max_quiet_moves) noexcept
    {
        if (max_quiet_moves < 0)
        {
            return;
        }

        MoveList pruned;
        int quiet_count = 0;

        for (int i = 0; i < list.count; ++i)
        {
            if (should_keep_after_prune(
                    list.moves[i],
                    quiet_count,
                    max_quiet_moves))
            {
                pruned.push(list.moves[i]);
            }
        }

        list = pruned;
    }

}
