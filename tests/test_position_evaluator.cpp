/**
 * @test ctest --test-dir build -R "^test_position_evaluator$" --output-on-failure
 */

#include "core/field.hpp"
#include "core/state.hpp"
#include "player/position_evaluator.hpp"

#include <gtest/gtest.h>

using ttt::game::Sign;
using ttt::game::State;
using ttt::my_player::EvalBreakdown;
using ttt::my_player::evaluate_breakdown;
using ttt::my_player::evaluate_position;
using ttt::my_player::kTerminalWinScore;
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

static SearchBoard MakeEmptyBoard(Sign my_sign = Sign::X)
{
    State state(DefaultOpts());
    return SearchBoard(state, my_sign);
}

TEST(PositionEvaluator, EmptyBoardHasZeroScore)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);

    EXPECT_EQ(evaluate_position(board, Sign::X), 0);
    EXPECT_EQ(evaluate_position(board, Sign::O), 0);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);
    EXPECT_EQ(breakdown.self_score, 0);
    EXPECT_EQ(breakdown.opponent_score, 0);
    EXPECT_EQ(breakdown.final_score, 0);
}

TEST(PositionEvaluator, DefaultOverloadUsesBoardMySign)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(6, 10, SearchBoard::Cell::X);
    board.set_cell(7, 10, SearchBoard::Cell::X);

    EXPECT_EQ(evaluate_position(board), evaluate_position(board, Sign::X));

    const EvalBreakdown a = evaluate_breakdown(board);
    const EvalBreakdown b = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(a.final_score, b.final_score);
    EXPECT_EQ(a.self.open_three, b.self.open_three);
}

TEST(PositionEvaluator, OpenFourIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(6, 10, SearchBoard::Cell::X);
    board.set_cell(7, 10, SearchBoard::Cell::X);
    board.set_cell(8, 10, SearchBoard::Cell::X);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(breakdown.self.open_four, 1);
    EXPECT_EQ(breakdown.self.simple_four, 0);
    EXPECT_EQ(breakdown.self.five, 0);
    EXPECT_GT(breakdown.self_score, 0);
}

TEST(PositionEvaluator, SimpleFourWithGapIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(6, 10, SearchBoard::Cell::X);
    board.set_cell(8, 10, SearchBoard::Cell::X);
    board.set_cell(9, 10, SearchBoard::Cell::X);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(breakdown.self.simple_four, 1);
    EXPECT_EQ(breakdown.self.open_four, 0);
    EXPECT_EQ(breakdown.self.five, 0);
}

TEST(PositionEvaluator, OpenThreeIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(6, 10, SearchBoard::Cell::X);
    board.set_cell(7, 10, SearchBoard::Cell::X);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(breakdown.self.open_three, 1);
    EXPECT_EQ(breakdown.self.simple_three, 0);
    EXPECT_EQ(breakdown.self.broken_three, 0);
}

TEST(PositionEvaluator, BrokenThreeIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(6, 10, SearchBoard::Cell::X);
    board.set_cell(8, 10, SearchBoard::Cell::X);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(breakdown.self.broken_three, 1);
}

TEST(PositionEvaluator, WallClosesOpenTwo)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(4, 10, SearchBoard::Cell::WALL);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(6, 10, SearchBoard::Cell::X);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(breakdown.self.open_two, 0);
    EXPECT_EQ(breakdown.self.closed_two, 1);
}

TEST(PositionEvaluator, OpponentThreatGivesNegativeScore)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::O);
    board.set_cell(6, 10, SearchBoard::Cell::O);
    board.set_cell(7, 10, SearchBoard::Cell::O);
    board.set_cell(8, 10, SearchBoard::Cell::O);

    const auto score_for_x = evaluate_position(board, Sign::X);
    const auto score_for_o = evaluate_position(board, Sign::O);

    EXPECT_LT(score_for_x, 0);
    EXPECT_GT(score_for_o, 0);
    EXPECT_EQ(score_for_x, -score_for_o);
}

TEST(PositionEvaluator, OpenFourScoresHigherThanOpenThree)
{
    SearchBoard open_three_board = MakeEmptyBoard(Sign::X);
    open_three_board.set_cell(5, 10, SearchBoard::Cell::X);
    open_three_board.set_cell(6, 10, SearchBoard::Cell::X);
    open_three_board.set_cell(7, 10, SearchBoard::Cell::X);

    SearchBoard open_four_board = MakeEmptyBoard(Sign::X);
    open_four_board.set_cell(5, 10, SearchBoard::Cell::X);
    open_four_board.set_cell(6, 10, SearchBoard::Cell::X);
    open_four_board.set_cell(7, 10, SearchBoard::Cell::X);
    open_four_board.set_cell(8, 10, SearchBoard::Cell::X);

    EXPECT_GT(evaluate_position(open_four_board, Sign::X),
              evaluate_position(open_three_board, Sign::X));
}

TEST(PositionEvaluator, DiagonalPatternsAreAlsoCounted)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(3, 3, SearchBoard::Cell::X);
    board.set_cell(4, 4, SearchBoard::Cell::X);
    board.set_cell(5, 5, SearchBoard::Cell::X);

    const EvalBreakdown breakdown = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(breakdown.self.open_three, 1);
}

TEST(PositionEvaluator, TerminalWinReturnsLargePositiveScore)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 11, 10);
    state.process_move(Sign::O, 1, 0);
    state.process_move(Sign::X, 12, 10);
    state.process_move(Sign::O, 2, 0);
    state.process_move(Sign::X, 13, 10);
    state.process_move(Sign::O, 3, 0);
    state.process_move(Sign::X, 15, 15);
    state.process_move(Sign::O, 4, 0);

    SearchBoard board(state, Sign::X);

    EXPECT_EQ(board.winner(), Sign::O);
    EXPECT_EQ(evaluate_position(board, Sign::O), kTerminalWinScore);
    EXPECT_EQ(evaluate_position(board, Sign::X), -kTerminalWinScore);
}

TEST(PositionEvaluator, TerminalDrawReturnsZero)
{
    State::Opts opts = DefaultOpts();
    opts.max_moves = 2;

    State state(opts);
    state.process_move(Sign::X, 9, 9);
    state.process_move(Sign::O, 10, 10);

    SearchBoard board(state, Sign::X);

    EXPECT_EQ(board.winner(), Sign::NONE);
    EXPECT_EQ(evaluate_position(board, Sign::X), 0);
    EXPECT_EQ(evaluate_position(board, Sign::O), 0);
}

TEST(PositionEvaluator, SimpleThreeIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(0, 10, SearchBoard::Cell::X);
    board.set_cell(1, 10, SearchBoard::Cell::X);
    board.set_cell(2, 10, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(bd.self.simple_three, 1);
    EXPECT_EQ(bd.self.open_three, 0);
}

TEST(PositionEvaluator, BrokenTwoIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::X);
    board.set_cell(7, 10, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(bd.self.broken_two, 1);
    EXPECT_EQ(bd.self.open_two, 0);
}

TEST(PositionEvaluator, ClosedTwoWithGapIsDetected)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(0, 10, SearchBoard::Cell::X);
    board.set_cell(2, 10, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(bd.self.closed_two, 1);
    EXPECT_EQ(bd.self.broken_two, 0);
}

TEST(PositionEvaluator, FiveInRowDetectedOnLeaf)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);

    for (int x = 5; x <= 9; ++x)
        board.set_cell(x, 10, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_GE(bd.self.five, 1);
    EXPECT_GT(bd.self_score, 0);
}

TEST(PositionEvaluator, ObstacleBreaksPattern)
{
    SearchBoard board_clean = MakeEmptyBoard(Sign::X);
    SearchBoard board_blocked = MakeEmptyBoard(Sign::X);

    for (int x = 5; x <= 8; ++x)
    {
        board_clean.set_cell(x, 10, SearchBoard::Cell::X);
        board_blocked.set_cell(x, 10, SearchBoard::Cell::X);
    }
    board_blocked.set_cell(6, 10, SearchBoard::Cell::WALL);

    EXPECT_GT(evaluate_position(board_clean, Sign::X),
              evaluate_position(board_blocked, Sign::X));
}

TEST(PositionEvaluator, ObstacleAdjacentToRunClosesIt)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(5, 10, SearchBoard::Cell::WALL);
    board.set_cell(6, 10, SearchBoard::Cell::X);
    board.set_cell(7, 10, SearchBoard::Cell::X);
    board.set_cell(8, 10, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(bd.self.open_three, 0);
    EXPECT_EQ(bd.self.simple_three, 1);
}

TEST(PositionEvaluator, TerminalScoreIgnoresPatterns)
{
    State state(DefaultOpts());

    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 0, 0);
    state.process_move(Sign::X, 11, 10);
    state.process_move(Sign::O, 1, 0);
    state.process_move(Sign::X, 12, 10);
    state.process_move(Sign::O, 2, 0);
    state.process_move(Sign::X, 13, 10);
    state.process_move(Sign::O, 3, 0);
    state.process_move(Sign::X, 15, 15);
    state.process_move(Sign::O, 4, 0);

    SearchBoard board(state, Sign::X);
    const auto bd = evaluate_breakdown(board, Sign::O);

    ASSERT_EQ(board.game_status(), ttt::game::Status::ENDED);
    ASSERT_EQ(board.winner(), Sign::O);

    EXPECT_EQ(bd.final_score, kTerminalWinScore);
    EXPECT_EQ(bd.self_score, 0);
    EXPECT_EQ(bd.opponent_score, 0);
}

TEST(PositionEvaluator, CornerPatternCounted)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(0, 0, SearchBoard::Cell::X);
    board.set_cell(1, 0, SearchBoard::Cell::X);
    board.set_cell(2, 0, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_EQ(bd.self.simple_three, 1);
    EXPECT_EQ(bd.self.open_three, 0);
}

TEST(PositionEvaluator, EdgeVerticalPatternCounted)
{
    SearchBoard board = MakeEmptyBoard(Sign::X);
    board.set_cell(0, 0, SearchBoard::Cell::X);
    board.set_cell(0, 1, SearchBoard::Cell::X);
    board.set_cell(0, 2, SearchBoard::Cell::X);
    board.set_cell(0, 3, SearchBoard::Cell::X);

    const EvalBreakdown bd = evaluate_breakdown(board, Sign::X);

    EXPECT_GE(bd.self.open_four + bd.self.simple_four, 1);
}