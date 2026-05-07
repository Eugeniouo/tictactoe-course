/**
 * @test ctest --test-dir build -R "^test_negamax_search$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/move_generator.hpp"
#include "player/negamax_searcher.hpp"

#include <gtest/gtest.h>

using ttt::game::Sign;
using ttt::game::State;
using ttt::game::Status;
using ttt::my_player::EngineConfig;
using ttt::my_player::MoveGenerator;
using ttt::my_player::MoveList;
using ttt::my_player::NegamaxResult;
using ttt::my_player::NegamaxSearcher;
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

    MoveList root_moves(const SearchState &search_state, Sign side)
    {
        return MoveGenerator().generate(
            search_state,
            side,
            EngineConfig::kMaxQuietMoves);
    }
}

TEST(NegamaxSearch, EmptyRootListReturnsNoMove)
{
    State state(default_opts());
    SearchState search_state = make_search_state(state, Sign::X);

    const NegamaxResult result =
        NegamaxSearcher().find_best_move(search_state, Sign::X, MoveList{});

    EXPECT_FALSE(result.found);
    EXPECT_EQ(result.nodes, 0);
}

TEST(NegamaxSearch, EmptyBoardReturnsCentralMove)
{
    State state(default_opts());
    SearchState search_state = make_search_state(state, Sign::X);

    const NegamaxResult result =
        NegamaxSearcher().find_best_move(
            search_state,
            Sign::X,
            root_moves(search_state, Sign::X));

    ASSERT_TRUE(result.found);
    EXPECT_EQ(result.best_move.x, 10);
    EXPECT_EQ(result.best_move.y, 10);
    EXPECT_GT(result.nodes, 0);
}

TEST(NegamaxSearch, ChoosesImmediateWinningMove)
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

    const NegamaxResult result =
        NegamaxSearcher().find_best_move(
            search_state,
            Sign::X,
            root_moves(search_state, Sign::X));

    ASSERT_TRUE(result.found);
    EXPECT_TRUE((result.best_move.x == 4 && result.best_move.y == 10) ||
                (result.best_move.x == 9 && result.best_move.y == 10));
}

TEST(NegamaxSearch, BlocksOpponentImmediateWin)
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

    const NegamaxResult result =
        NegamaxSearcher().find_best_move(
            search_state,
            Sign::X,
            root_moves(search_state, Sign::X));

    ASSERT_TRUE(result.found);
    EXPECT_TRUE((result.best_move.x == 4 && result.best_move.y == 10) ||
                (result.best_move.x == 9 && result.best_move.y == 10));
}

TEST(NegamaxSearch, DoesNotModifyOriginalSearchState)
{
    State state(default_opts());
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 11, 10);

    SearchState search_state = make_search_state(state, Sign::X);
    const Sign current_player = search_state.board().current_player();
    const Status status = search_state.game_status();
    const int move_number = search_state.board().move_number();
    const std::uint64_t hash = search_state.board().hash();

    const NegamaxResult result =
        NegamaxSearcher().find_best_move(
            search_state,
            Sign::X,
            root_moves(search_state, Sign::X));

    EXPECT_TRUE(result.found);
    EXPECT_EQ(search_state.board().current_player(), current_player);
    EXPECT_EQ(search_state.game_status(), status);
    EXPECT_EQ(search_state.board().move_number(), move_number);
    EXPECT_EQ(search_state.board().hash(), hash);
    EXPECT_EQ(search_state.board().get_cell(10, 10), SearchBoard::Cell::X);
    EXPECT_EQ(search_state.board().get_cell(11, 10), SearchBoard::Cell::O);
}
