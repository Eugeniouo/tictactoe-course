/**
 * @test ctest --test-dir build -R "^test_instant_move_checker$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/instant_move_checker.hpp"

#include <gtest/gtest.h>

using ttt::game::Sign;
using ttt::game::State;
using ttt::game::Status;
using ttt::my_player::find_instant_move;
using ttt::my_player::InstantMoveKind;
using ttt::my_player::InstantMoveResult;
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

TEST(InstantMoveChecker, InvalidCurrentPlayerReturnsNoMove)
{
    SearchBoard board;

    const InstantMoveResult result = find_instant_move(board);

    EXPECT_EQ(result.move.x, -1);
    EXPECT_EQ(result.move.y, -1);
    EXPECT_EQ(result.kind, InstantMoveKind::NONE);
}

TEST(InstantMoveChecker, SignOFindsImmediateWinningMove)
{
    State state(DefaultOpts());

    EXPECT_EQ(state.process_move(Sign::X, 0, 0), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::O, 5, 10), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::X, 1, 0), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::O, 6, 10), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::X, 2, 0), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::O, 7, 10), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::X, 3, 1), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::O, 8, 10), ttt::game::MoveResult::OK);
    EXPECT_EQ(state.process_move(Sign::X, 4, 1), ttt::game::MoveResult::OK);

    SearchBoard board(state, Sign::O);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_TRUE((result.move.x == 4 && result.move.y == 10) ||
                (result.move.x == 9 && result.move.y == 10));
    EXPECT_EQ(result.kind, InstantMoveKind::WIN);
}

TEST(InstantMoveChecker, BlocksOpponentImmediateWinningMove)
{
    State state(DefaultOpts());
    state.process_move(Sign::X, 4, 10);
    state.process_move(Sign::O, 5, 10);
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 6, 10);
    state.process_move(Sign::X, 10, 11);
    state.process_move(Sign::O, 7, 10);
    state.process_move(Sign::X, 11, 10);
    state.process_move(Sign::O, 8, 10);

    SearchBoard board(state, Sign::X);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_EQ(result.move.x, 9);
    EXPECT_EQ(result.move.y, 10);
    EXPECT_EQ(result.kind, InstantMoveKind::BLOCK);
}

TEST(InstantMoveChecker, PrefersOwnTacticalResultOverBlocking)
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

    SearchBoard board(state, Sign::X);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_TRUE((result.move.x == 4 && result.move.y == 10) ||
                (result.move.x == 9 && result.move.y == 10))
        << "X must play its own tactical move first; got ("
        << result.move.x << "," << result.move.y << ")";
    EXPECT_EQ(result.kind, InstantMoveKind::FORCE_LAST_MOVE);
}

TEST(InstantMoveChecker, LastMovePositionFindsDrawingReply)
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
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_TRUE((result.move.x == 4 && result.move.y == 12) ||
                (result.move.x == 9 && result.move.y == 12));
    EXPECT_EQ(result.kind, InstantMoveKind::DRAW_SAVE);
}

TEST(InstantMoveChecker, DoesNotModifyOriginalBoard)
{
    State state(DefaultOpts());
    state.process_move(Sign::X, 4, 10);
    state.process_move(Sign::O, 5, 10);
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 6, 10);
    state.process_move(Sign::X, 10, 11);
    state.process_move(Sign::O, 7, 10);
    state.process_move(Sign::X, 11, 10);
    state.process_move(Sign::O, 8, 10);

    SearchBoard board(state, Sign::X);
    const Sign current_player = board.current_player();
    const Status status = board.game_status();
    const int move_number = board.move_number();

    const InstantMoveResult result = find_instant_move(board);

    EXPECT_NE(result.kind, InstantMoveKind::NONE);
    EXPECT_EQ(board.current_player(), current_player);
    EXPECT_EQ(board.game_status(), status);
    EXPECT_EQ(board.move_number(), move_number);
    EXPECT_EQ(board.get_cell(4, 10), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(10, 10), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(8, 10), SearchBoard::Cell::O);
    EXPECT_EQ(board.get_cell(9, 10), SearchBoard::Cell::EMPTY);
}

TEST(InstantMoveChecker, SignXFindsImmediateWinningMove)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 5, 5);
    state.process_move(Sign::O, 0, 19);
    state.process_move(Sign::X, 6, 5);
    state.process_move(Sign::O, 1, 19);
    state.process_move(Sign::X, 7, 5);
    state.process_move(Sign::O, 2, 19);
    state.process_move(Sign::X, 8, 5);
    state.process_move(Sign::O, 3, 19);

    SearchBoard board(state, Sign::X);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_TRUE((result.move.x == 4 && result.move.y == 5) ||
                (result.move.x == 9 && result.move.y == 5))
        << "X must find the winning move; got ("
        << result.move.x << "," << result.move.y << ")";

    EXPECT_NE(result.kind, InstantMoveKind::NONE);
    EXPECT_NE(result.kind, InstantMoveKind::BLOCK);
}

TEST(InstantMoveChecker, NoThreatsEarlyGameReturnsNoMove)
{
    State state(DefaultOpts());
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 3, 3);
    state.process_move(Sign::X, 15, 15);
    state.process_move(Sign::O, 7, 12);

    SearchBoard board(state, Sign::X);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_EQ(result.move.x, -1);
    EXPECT_EQ(result.move.y, -1);
    EXPECT_EQ(result.kind, InstantMoveKind::NONE);
}

TEST(InstantMoveChecker, CannotBlockDoubleAttackReturnsNoMove)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 0, 0);
    state.process_move(Sign::O, 5, 10);
    state.process_move(Sign::X, 19, 19);
    state.process_move(Sign::O, 6, 10);
    state.process_move(Sign::X, 0, 19);
    state.process_move(Sign::O, 7, 10);
    state.process_move(Sign::X, 19, 0);
    state.process_move(Sign::O, 8, 10);

    state.process_move(Sign::X, 3, 7);
    state.process_move(Sign::O, 5, 15);
    state.process_move(Sign::X, 12, 3);
    state.process_move(Sign::O, 6, 15);
    state.process_move(Sign::X, 7, 1);
    state.process_move(Sign::O, 7, 15);
    state.process_move(Sign::X, 14, 6);
    state.process_move(Sign::O, 8, 15);

    SearchBoard board(state, Sign::X);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_EQ(result.kind, InstantMoveKind::NONE)
        << "With two independent open fours, no single move can block both";
}

TEST(InstantMoveChecker, EndedPositionReturnsNoMove)
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
    state.process_move(Sign::X, 9, 10);
    state.process_move(Sign::O, 0, 0);

    ASSERT_EQ(state.get_status(), Status::ENDED);

    SearchBoard board(state, Sign::X);
    const InstantMoveResult result = find_instant_move(board);

    EXPECT_EQ(result.move.x, -1);
    EXPECT_EQ(result.move.y, -1);
    EXPECT_EQ(result.kind, InstantMoveKind::NONE);
}