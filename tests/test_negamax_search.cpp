/**
 * @test ctest --test-dir build -R "^test_negamax_search$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/negamax_search.hpp"
#include "player/position_evaluator.hpp"

#include <gtest/gtest.h>

using ttt::game::Point;
using ttt::game::Sign;
using ttt::game::State;
using ttt::game::Status;
using ttt::my_player::evaluate_position;
using ttt::my_player::find_best_move;
using ttt::my_player::kTerminalWinScore;
using ttt::my_player::SearchBoard;
using ttt::my_player::SearchResult;

static State::Opts DefaultOpts()
{
    State::Opts opts{};
    opts.rows = SearchBoard::kBoardHeight;
    opts.cols = SearchBoard::kBoardWidth;
    opts.win_len = SearchBoard::kWinLength;
    opts.max_moves = 0;
    return opts;
}

TEST(NegamaxSearch, InvalidCurrentPlayerReturnsNoMove)
{
    SearchBoard board;

    const SearchResult result = find_best_move(board, 1);

    EXPECT_EQ(result.best_move.x, -1);
    EXPECT_EQ(result.best_move.y, -1);
    EXPECT_EQ(result.best_score, 0);
    EXPECT_EQ(result.visited_nodes, 0u);
}

TEST(NegamaxSearch, DepthZeroReturnsStaticEvaluationWithoutMove)
{
    State state(DefaultOpts());
    SearchBoard board(state, Sign::X);

    const SearchResult result = find_best_move(board, 0);

    EXPECT_EQ(result.best_move.x, -1);
    EXPECT_EQ(result.best_move.y, -1);
    EXPECT_EQ(result.best_score, evaluate_position(board, Sign::X));
    EXPECT_EQ(result.visited_nodes, 1u);
}

TEST(NegamaxSearch, EmptyBoardReturnsCentralMove)
{
    State state(DefaultOpts());
    SearchBoard board(state, Sign::X);

    const SearchResult result = find_best_move(board, 1);

    EXPECT_TRUE((result.best_move.x == 9 || result.best_move.x == 10) &&
                (result.best_move.y == 9 || result.best_move.y == 10));
    EXPECT_GT(result.visited_nodes, 0u);
}

TEST(NegamaxSearch, TerminalPositionReturnsNoMove)
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
    const SearchResult result = find_best_move(board, 2);

    ASSERT_EQ(board.game_status(), Status::ENDED);
    ASSERT_EQ(board.winner(), Sign::O);

    EXPECT_EQ(result.best_move.x, -1);
    EXPECT_EQ(result.best_move.y, -1);
    EXPECT_EQ(result.best_score, -kTerminalWinScore);
    EXPECT_EQ(result.visited_nodes, 1u);
}

TEST(NegamaxSearch, ChoosesImmediateWinningMove)
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
    const SearchResult result = find_best_move(board, 1);

    EXPECT_TRUE((result.best_move.x == 4 && result.best_move.y == 10) ||
                (result.best_move.x == 9 && result.best_move.y == 10));
    EXPECT_GT(result.best_score, 0);
}

TEST(NegamaxSearch, BlocksOpponentOpenFour)
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
    const SearchResult result = find_best_move(board, 1);

    EXPECT_TRUE((result.best_move.x == 4 && result.best_move.y == 10) ||
                (result.best_move.x == 9 && result.best_move.y == 10));
}

TEST(NegamaxSearch, DoesNotModifyOriginalBoard)
{
    State state(DefaultOpts());
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 11, 10);

    SearchBoard board(state, Sign::X);
    const Sign current_player = board.current_player();
    const Status status = board.game_status();

    const SearchResult result = find_best_move(board, 1);

    EXPECT_NE(result.best_move.x, -1);
    EXPECT_EQ(board.current_player(), current_player);
    EXPECT_EQ(board.game_status(), status);
    EXPECT_EQ(board.get_cell(10, 10), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(11, 10), SearchBoard::Cell::O);
}

TEST(NegamaxSearch, PrefersWinOverBlock)
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
    const SearchResult result = find_best_move(board, 1);

    EXPECT_TRUE((result.best_move.x == 4 && result.best_move.y == 10) ||
                (result.best_move.x == 9 && result.best_move.y == 10))
        << "X must win instead of blocking; got ("
        << result.best_move.x << "," << result.best_move.y << ")";
    EXPECT_GT(result.best_score, 0);
}

TEST(NegamaxSearch, SignOChoosesImmediateWin)
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
    const SearchResult result = find_best_move(board, 1);

    EXPECT_TRUE((result.best_move.x == 4 && result.best_move.y == 10) ||
                (result.best_move.x == 9 && result.best_move.y == 10))
        << "O must complete five-in-a-row; got ("
        << result.best_move.x << "," << result.best_move.y << ")";
    EXPECT_GT(result.best_score, 0);
}
