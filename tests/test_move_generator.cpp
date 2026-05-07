/**
 * @test ctest --test-dir build -R "^test_move_generator$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/move_generator.hpp"

#include <gtest/gtest.h>

using ttt::game::Sign;
using ttt::game::State;
using ttt::my_player::MoveGenerator;
using ttt::my_player::MoveList;
using ttt::my_player::MoveTier;
using ttt::my_player::SearchBoard;
using ttt::my_player::SearchState;

namespace
{
    State::Opts default_opts()
    {
        State::Opts opts{};
        opts.rows = SearchBoard::kBoardHeight;
        opts.cols = SearchBoard::kBoardWidth;
        opts.win_len = SearchBoard::kWinLength;
        opts.max_moves = 0;
        return opts;
    }

    SearchState make_search_state(const State &state, Sign my_sign)
    {
        SearchState search_state;
        search_state.load_from_state(state, my_sign);
        return search_state;
    }

    bool contains_move(const MoveList &moves, int x, int y)
    {
        for (int i = 0; i < moves.count; ++i)
        {
            if (moves[i].point.x == x && moves[i].point.y == y)
            {
                return true;
            }
        }

        return false;
    }

    int count_move(const MoveList &moves, int x, int y)
    {
        int count = 0;

        for (int i = 0; i < moves.count; ++i)
        {
            if (moves[i].point.x == x && moves[i].point.y == y)
            {
                ++count;
            }
        }

        return count;
    }
}

TEST(MoveGenerator, EmptyBoardReturnsSingleCentralMove)
{
    State state(default_opts());
    SearchState search_state = make_search_state(state, Sign::X);

    const MoveList moves =
        MoveGenerator().generate(search_state, Sign::X, 6);

    ASSERT_EQ(moves.count, 1);
    EXPECT_EQ(moves[0].point.x, 10);
    EXPECT_EQ(moves[0].point.y, 10);
    EXPECT_EQ(moves[0].tier, MoveTier::QUIET);
}

TEST(MoveGenerator, SingleStoneGeneratesRadiusThreeNeighborhood)
{
    State state(default_opts());
    state.process_move(Sign::X, 10, 10);

    SearchState search_state = make_search_state(state, Sign::O);

    const MoveList moves =
        MoveGenerator().generate(search_state, Sign::O, -1);

    EXPECT_EQ(moves.count, 48);
    EXPECT_FALSE(contains_move(moves, 10, 10));
    EXPECT_TRUE(contains_move(moves, 7, 7));
    EXPECT_TRUE(contains_move(moves, 13, 13));
    EXPECT_TRUE(contains_move(moves, 10, 7));
    EXPECT_TRUE(contains_move(moves, 13, 10));
}

TEST(MoveGenerator, CornerNeighborhoodIsClippedToBoard)
{
    State state(default_opts());
    state.process_move(Sign::X, 0, 0);

    SearchState search_state = make_search_state(state, Sign::O);

    const MoveList moves =
        MoveGenerator().generate(search_state, Sign::O, -1);

    EXPECT_EQ(moves.count, 15);
    EXPECT_FALSE(contains_move(moves, 0, 0));
    EXPECT_TRUE(contains_move(moves, 0, 1));
    EXPECT_TRUE(contains_move(moves, 1, 0));
    EXPECT_TRUE(contains_move(moves, 3, 3));
}

TEST(MoveGenerator, DoesNotDuplicateOverlappingNeighborhoods)
{
    State state(default_opts());
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 11, 10);

    SearchState search_state = make_search_state(state, Sign::X);

    const MoveList moves =
        MoveGenerator().generate(search_state, Sign::X, -1);

    EXPECT_FALSE(contains_move(moves, 10, 10));
    EXPECT_FALSE(contains_move(moves, 11, 10));
    EXPECT_EQ(count_move(moves, 9, 10), 1);
}

TEST(MoveGenerator, WinningMoveGoesFirst)
{
    State state(default_opts());
    state.process_move(Sign::X, 5, 10);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 6, 10);
    state.process_move(Sign::O, 0, 1);
    state.process_move(Sign::X, 7, 10);
    state.process_move(Sign::O, 0, 2);
    state.process_move(Sign::X, 8, 10);
    state.process_move(Sign::O, 1, 0);

    SearchState search_state = make_search_state(state, Sign::X);

    const MoveList moves =
        MoveGenerator().generate(search_state, Sign::X, 0);

    ASSERT_FALSE(moves.empty());
    EXPECT_EQ(moves[0].tier, MoveTier::WIN);
    EXPECT_TRUE((moves[0].point.x == 4 && moves[0].point.y == 10) ||
                (moves[0].point.x == 9 && moves[0].point.y == 10));
}

TEST(MoveGenerator, BlockingMoveSurvivesQuietPruning)
{
    State state(default_opts());
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 5, 10);
    state.process_move(Sign::X, 10, 11);
    state.process_move(Sign::O, 6, 10);
    state.process_move(Sign::X, 11, 10);
    state.process_move(Sign::O, 7, 10);
    state.process_move(Sign::X, 11, 11);
    state.process_move(Sign::O, 8, 10);

    SearchState search_state = make_search_state(state, Sign::X);

    const MoveList moves =
        MoveGenerator().generate(search_state, Sign::X, 0);

    ASSERT_FALSE(moves.empty());
    EXPECT_EQ(moves[0].tier, MoveTier::BLOCK_WIN);
    EXPECT_TRUE((moves[0].point.x == 4 && moves[0].point.y == 10) ||
                (moves[0].point.x == 9 && moves[0].point.y == 10));
}
