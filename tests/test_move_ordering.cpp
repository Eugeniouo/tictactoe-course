/**
 * @test ctest --test-dir build -R "^test_move_ordering$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/move_ordering.hpp"

#include <gtest/gtest.h>

using ttt::game::Point;
using ttt::game::Sign;
using ttt::game::State;
using ttt::game::Status;
using ttt::my_player::MoveList;
using ttt::my_player::order_candidate_moves;
using ttt::my_player::SearchBoard;

static State::Opts DefaultOpts()
{
    State::Opts opts{};
    opts.rows = SearchBoard::kBoardHeight;
    opts.cols = SearchBoard::kBoardWidth;
    opts.win_len = SearchBoard::kWinLength;
    opts.max_moves = 0;
    return opts;
}

TEST(MoveOrdering, ImmediateWinningMoveGoesBeforeQuietMove)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 5, 10);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 6, 10);
    state.process_move(Sign::O, 0, 1);
    state.process_move(Sign::X, 7, 10);
    state.process_move(Sign::O, 0, 2);
    state.process_move(Sign::X, 8, 10);
    state.process_move(Sign::O, 1, 0);

    SearchBoard board(state, Sign::X);
    MoveList moves{{10, 11}, {9, 10}};

    order_candidate_moves(board, moves);

    ASSERT_EQ(moves.size(), 2u);
    EXPECT_EQ(moves[0].x, 9);
    EXPECT_EQ(moves[0].y, 10);
}

TEST(MoveOrdering, BlockingMoveGoesBeforeQuietMove)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 5, 10);
    state.process_move(Sign::X, 10, 11);
    state.process_move(Sign::O, 6, 10);
    state.process_move(Sign::X, 11, 10);
    state.process_move(Sign::O, 7, 10);
    state.process_move(Sign::X, 11, 11);
    state.process_move(Sign::O, 8, 10);

    SearchBoard board(state, Sign::X);
    MoveList moves{{12, 12}, {9, 10}};

    order_candidate_moves(board, moves);

    ASSERT_EQ(moves.size(), 2u);
    EXPECT_EQ(moves[0].x, 9);
    EXPECT_EQ(moves[0].y, 10);
}

TEST(MoveOrdering, OpenFourCreatingMoveGoesBeforeQuietMove)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 5, 10);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 6, 10);
    state.process_move(Sign::O, 0, 1);
    state.process_move(Sign::X, 7, 10);
    state.process_move(Sign::O, 1, 0);

    SearchBoard board(state, Sign::X);
    MoveList moves{{10, 10}, {8, 10}};

    order_candidate_moves(board, moves);

    ASSERT_EQ(moves.size(), 2u);
    EXPECT_EQ(moves[0].x, 8);
    EXPECT_EQ(moves[0].y, 10);
}

TEST(MoveOrdering, LastMoveDrawingReplyGoesBeforeQuietMove)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 5, 10);
    state.process_move(Sign::O, 5, 12);
    state.process_move(Sign::X, 6, 10);
    state.process_move(Sign::O, 6, 12);
    state.process_move(Sign::X, 7, 10);
    state.process_move(Sign::O, 7, 12);
    state.process_move(Sign::X, 8, 10);
    state.process_move(Sign::O, 8, 12);
    EXPECT_EQ(state.process_move(Sign::X, 9, 10), ttt::game::MoveResult::OK);

    ASSERT_EQ(state.get_status(), Status::LAST_MOVE);
    ASSERT_EQ(state.get_current_player(), Sign::O);

    SearchBoard board(state, Sign::O);
    MoveList moves{{10, 10}, {9, 12}};

    order_candidate_moves(board, moves);

    ASSERT_EQ(moves.size(), 2u);
    EXPECT_EQ(moves[0].x, 9);
    EXPECT_EQ(moves[0].y, 12);
}

TEST(MoveOrdering, EqualScoresPreserveOriginalOrder)
{
    State state(DefaultOpts());
    SearchBoard board(state, Sign::X);

    MoveList moves{{9, 9}, {10, 10}};

    order_candidate_moves(board, moves);

    ASSERT_EQ(moves.size(), 2u);
    EXPECT_EQ(moves[0].x, 9);
    EXPECT_EQ(moves[0].y, 9);
    EXPECT_EQ(moves[1].x, 10);
    EXPECT_EQ(moves[1].y, 10);
}

TEST(MoveOrdering, DoesNotModifyBoard)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 5, 10);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 6, 10);
    state.process_move(Sign::O, 0, 1);
    state.process_move(Sign::X, 7, 10);
    state.process_move(Sign::O, 1, 0);

    SearchBoard board(state, Sign::X);

    const Sign current_player = board.current_player();
    const Status status = board.game_status();
    const int move_number = board.move_number();

    MoveList moves{{10, 10}, {8, 10}};
    order_candidate_moves(board, moves);

    EXPECT_EQ(board.current_player(), current_player);
    EXPECT_EQ(board.game_status(), status);
    EXPECT_EQ(board.move_number(), move_number);
    EXPECT_EQ(board.get_cell(5, 10), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(6, 10), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(7, 10), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(0, 0), SearchBoard::Cell::O);
    EXPECT_EQ(board.get_cell(0, 1), SearchBoard::Cell::O);
    EXPECT_EQ(board.get_cell(1, 0), SearchBoard::Cell::O);
    EXPECT_EQ(board.get_cell(8, 10), SearchBoard::Cell::EMPTY);
    EXPECT_EQ(board.get_cell(10, 10), SearchBoard::Cell::EMPTY);
}
