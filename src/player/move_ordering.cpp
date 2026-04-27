#include "move_ordering.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace ttt::my_player
{
    namespace
    {
        using OrderingScore = std::int64_t;
        using Cell = SearchBoard::Cell;

        constexpr int kNoExtraOffset = 1000;

        struct Direction
        {
            int dx = 0;
            int dy = 0;
        };

        constexpr std::array<Direction, 4> kDirections{{
            {1, 0},
            {0, 1},
            {1, 1},
            {1, -1},
        }};

        struct LocalPatternStats
        {
            int five = 0;
            int open_four = 0;
            int simple_four = 0;
            int open_three = 0;
            int broken_three = 0;
        };

        struct ScoredMove
        {
            game::Point move;
            OrderingScore score = 0;
            std::size_t original_index = 0;
        };

        bool is_player_sign(game::Sign sign)
        {
            return sign == game::Sign::X || sign == game::Sign::O;
        }

        bool is_stone_cell(Cell cell)
        {
            return cell == Cell::X || cell == Cell::O;
        }

        int abs_int(int value)
        {
            return value < 0 ? -value : value;
        }

        int max_int(int lhs, int rhs)
        {
            return lhs > rhs ? lhs : rhs;
        }

        bool is_real_empty_cell(
            const SearchBoard &board,
            const game::Point &move,
            Direction dir,
            int offset)
        {
            const int x = move.x + dir.dx * offset;
            const int y = move.y + dir.dy * offset;

            return board.get_cell(x, y) == Cell::EMPTY;
        }

        bool is_empty_with_hypothesis(
            const SearchBoard &board,
            const game::Point &move,
            Direction dir,
            int offset,
            int extra_offset)
        {
            if (offset == 0 || offset == extra_offset)
            {
                return false;
            }

            return is_real_empty_cell(board, move, dir, offset);
        }

        bool is_stone_with_hypothesis(
            const SearchBoard &board,
            const game::Point &move,
            Direction dir,
            int offset,
            Cell stone,
            int extra_offset_a = kNoExtraOffset,
            int extra_offset_b = kNoExtraOffset)
        {
            if (offset == 0 ||
                offset == extra_offset_a ||
                offset == extra_offset_b)
            {
                return true;
            }

            const int x = move.x + dir.dx * offset;
            const int y = move.y + dir.dy * offset;

            return board.get_cell(x, y) == stone;
        }

        bool has_five_in_line(
            const SearchBoard &board,
            const game::Point &move,
            Direction dir,
            Cell stone,
            int extra_offset_a = kNoExtraOffset,
            int extra_offset_b = kNoExtraOffset)
        {
            for (int start = -4; start <= 0; ++start)
            {
                bool all_stones = true;

                for (int offset = start; offset < start + 5; ++offset)
                {
                    if (!is_stone_with_hypothesis(
                            board,
                            move,
                            dir,
                            offset,
                            stone,
                            extra_offset_a,
                            extra_offset_b))
                    {
                        all_stones = false;
                        break;
                    }
                }

                if (all_stones)
                {
                    return true;
                }
            }

            return false;
        }

        int count_winning_replies_in_line(
            const SearchBoard &board,
            const game::Point &move,
            Direction dir,
            Cell stone,
            int extra_offset = kNoExtraOffset)
        {
            int winning_replies = 0;

            for (int offset = -4; offset <= 4; ++offset)
            {
                if (!is_empty_with_hypothesis(
                        board,
                        move,
                        dir,
                        offset,
                        extra_offset))
                {
                    continue;
                }

                if (has_five_in_line(
                        board,
                        move,
                        dir,
                        stone,
                        extra_offset,
                        offset))
                {
                    ++winning_replies;
                }
            }

            return winning_replies;
        }

        int count_four_creating_replies_in_line(
            const SearchBoard &board,
            const game::Point &move,
            Direction dir,
            Cell stone)
        {
            int four_creating_replies = 0;

            for (int offset = -4; offset <= 4; ++offset)
            {
                if (!is_empty_with_hypothesis(
                        board,
                        move,
                        dir,
                        offset,
                        kNoExtraOffset))
                {
                    continue;
                }

                const int winning_replies_after_extra_stone =
                    count_winning_replies_in_line(
                        board,
                        move,
                        dir,
                        stone,
                        offset);

                if (winning_replies_after_extra_stone > 0)
                {
                    ++four_creating_replies;
                }
            }

            return four_creating_replies;
        }

        LocalPatternStats analyze_local_patterns(
            const SearchBoard &board,
            const game::Point &move,
            game::Sign sign)
        {
            LocalPatternStats stats{};
            const Cell stone = SearchBoard::cell_from_game_sign(sign);

            if (!is_stone_cell(stone))
            {
                return stats;
            }

            for (const Direction dir : kDirections)
            {
                if (has_five_in_line(board, move, dir, stone))
                {
                    ++stats.five;
                    continue;
                }

                const int winning_replies =
                    count_winning_replies_in_line(board, move, dir, stone);

                if (winning_replies >= 2)
                {
                    ++stats.open_four;
                    continue;
                }

                if (winning_replies == 1)
                {
                    ++stats.simple_four;
                    continue;
                }

                const int four_creating_replies =
                    count_four_creating_replies_in_line(board, move, dir, stone);

                if (four_creating_replies >= 2)
                {
                    ++stats.open_three;
                }
                else if (four_creating_replies == 1)
                {
                    ++stats.broken_three;
                }
            }

            return stats;
        }

        int local_activity_score(const SearchBoard &board, const game::Point &move)
        {
            int score = 0;

            for (int dy = -2; dy <= 2; ++dy)
            {
                for (int dx = -2; dx <= 2; ++dx)
                {
                    if (dx == 0 && dy == 0)
                    {
                        continue;
                    }

                    const Cell cell = board.get_cell(move.x + dx, move.y + dy);
                    if (!is_stone_cell(cell))
                    {
                        continue;
                    }

                    const int distance = max_int(abs_int(dx), abs_int(dy));

                    if (distance == 1)
                    {
                        score += 8;
                    }
                    else if (distance == 2)
                    {
                        score += 3;
                    }
                }
            }

            return score;
        }

        OrderingScore score_move_fast(
            const SearchBoard &board,
            const game::Point &move,
            game::Sign player)
        {
            const game::Sign opponent = SearchBoard::opposite_player_sign(player);

            const LocalPatternStats self =
                analyze_local_patterns(board, move, player);

            const LocalPatternStats opponent_threat =
                analyze_local_patterns(board, move, opponent);

            const bool is_last_move_response =
                board.game_status() == game::Status::LAST_MOVE;

            OrderingScore score = 0;

            if (is_last_move_response && self.five > 0)
            {
                score += 4'000'000'000'000LL;
            }

            score += static_cast<OrderingScore>(self.five) *
                     3'000'000'000'000LL;

            score += static_cast<OrderingScore>(opponent_threat.five) *
                     2'500'000'000'000LL;

            score += static_cast<OrderingScore>(self.open_four) *
                     500'000'000'000LL;

            score += static_cast<OrderingScore>(opponent_threat.open_four) *
                     450'000'000'000LL;

            score += static_cast<OrderingScore>(self.simple_four) *
                     120'000'000'000LL;

            score += static_cast<OrderingScore>(opponent_threat.simple_four) *
                     100'000'000'000LL;

            score += static_cast<OrderingScore>(self.open_three) *
                     20'000'000'000LL;

            score += static_cast<OrderingScore>(opponent_threat.open_three) *
                     16'000'000'000LL;

            score += static_cast<OrderingScore>(self.broken_three) *
                     5'000'000'000LL;

            score += static_cast<OrderingScore>(opponent_threat.broken_three) *
                     4'000'000'000LL;

            score += static_cast<OrderingScore>(local_activity_score(board, move)) *
                     1'000'000LL;

            return score;
        }

    } // namespace

    void order_candidate_moves(SearchBoard &board, MoveList &moves)
    {
        if (moves.size() < 2)
        {
            return;
        }

        const game::Sign player = board.current_player();
        if (!is_player_sign(player))
        {
            return;
        }

        std::vector<ScoredMove> scored_moves;
        scored_moves.reserve(moves.size());

        for (std::size_t i = 0; i < moves.size(); ++i)
        {
            scored_moves.push_back({
                moves[i],
                score_move_fast(board, moves[i], player),
                i,
            });
        }

        std::sort(
            scored_moves.begin(),
            scored_moves.end(),
            [](const ScoredMove &lhs, const ScoredMove &rhs)
            {
                if (lhs.score != rhs.score)
                {
                    return lhs.score > rhs.score;
                }

                return lhs.original_index < rhs.original_index;
            });

        for (std::size_t i = 0; i < scored_moves.size(); ++i)
        {
            moves[i] = scored_moves[i].move;
        }
    }

} // namespace ttt::my_player
