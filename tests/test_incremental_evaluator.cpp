/**
 * @test ctest --test-dir build -R "^test_incremental_evaluator$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/search_state.hpp"

#include <gtest/gtest.h>

using ttt::game::Sign;
using ttt::game::State;
using ttt::my_player::MoveClass;
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
}

TEST(IncrementalEvaluator, EmptyBoardHasZeroScore)
{
    State state(default_opts());
    SearchState search_state = make_search_state(state, Sign::X);

    EXPECT_EQ(search_state.evaluator().evaluate(Sign::X), 0);
    EXPECT_EQ(search_state.evaluator().evaluate(Sign::O), 0);
}

TEST(IncrementalEvaluator, WinningExtensionIsClassifiedAsWin)
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

    EXPECT_EQ(
        search_state.evaluator().move_class(
            SearchBoard::to_index(4, 10),
            Sign::X),
        MoveClass::WIN);
    EXPECT_EQ(
        search_state.evaluator().move_class(
            SearchBoard::to_index(9, 10),
            Sign::X),
        MoveClass::WIN);
}

TEST(IncrementalEvaluator, OpponentWinningMoveIsVisibleForDefense)
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

    EXPECT_EQ(
        search_state.evaluator().move_class(
            SearchBoard::to_index(4, 10),
            Sign::O),
        MoveClass::WIN);
    EXPECT_EQ(
        search_state.evaluator().move_class(
            SearchBoard::to_index(9, 10),
            Sign::O),
        MoveClass::WIN);
}

TEST(IncrementalEvaluator, OpenFourScoresHigherThanOpenThree)
{
    SearchState open_three = make_search_state(State(default_opts()), Sign::X);
    open_three.apply_move(5, 10, Sign::X);
    open_three.apply_move(6, 10, Sign::X);
    open_three.apply_move(7, 10, Sign::X);

    SearchState open_four = make_search_state(State(default_opts()), Sign::X);
    open_four.apply_move(5, 10, Sign::X);
    open_four.apply_move(6, 10, Sign::X);
    open_four.apply_move(7, 10, Sign::X);
    open_four.apply_move(8, 10, Sign::X);

    EXPECT_GT(
        open_four.evaluator().evaluate(Sign::X),
        open_three.evaluator().evaluate(Sign::X));
}

TEST(IncrementalEvaluator, ApplyAndUndoRestoreScoresAndCells)
{
    State state(default_opts());
    SearchState search_state = make_search_state(state, Sign::X);

    const auto score_before = search_state.evaluator().evaluate(Sign::X);
    const auto hash_before = search_state.board().hash();

    const SearchState::Undo first = search_state.apply_move(5, 10, Sign::X);
    const SearchState::Undo second = search_state.apply_move(6, 10, Sign::X);

    EXPECT_EQ(search_state.board().get_cell(5, 10), SearchBoard::Cell::X);
    EXPECT_EQ(search_state.board().get_cell(6, 10), SearchBoard::Cell::X);
    EXPECT_NE(search_state.board().hash(), hash_before);

    search_state.undo_move(second);
    search_state.undo_move(first);

    EXPECT_EQ(search_state.board().get_cell(5, 10), SearchBoard::Cell::EMPTY);
    EXPECT_EQ(search_state.board().get_cell(6, 10), SearchBoard::Cell::EMPTY);
    EXPECT_EQ(search_state.evaluator().evaluate(Sign::X), score_before);
    EXPECT_EQ(search_state.board().hash(), hash_before);
}
