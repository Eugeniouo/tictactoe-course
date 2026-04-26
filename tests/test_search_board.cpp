/**
 * @test ctest --test-dir build -R "^test_search_board$" --output-on-failure
 */

#include "core/field.hpp"
#include "core/state.hpp"
#include "player/search_board.hpp"

#include <gtest/gtest.h>

using ttt::game::MoveResult;
using ttt::game::Sign;
using ttt::game::State;
using ttt::game::Status;
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

static State MakeActiveState()
{
  State state(DefaultOpts());
  state.process_move(Sign::X, 0, 0);
  state.process_move(Sign::O, 2, 3);
  state.process_move(Sign::X, 5, 5);
  state.process_move(Sign::O, 7, 8);
  return state;
}

TEST(SearchBoard, IsWithinBoard)
{
  EXPECT_TRUE(SearchBoard::is_within_board(0, 0));
  EXPECT_TRUE(SearchBoard::is_within_board(19, 19));
  EXPECT_FALSE(SearchBoard::is_within_board(-1, 0));
  EXPECT_FALSE(SearchBoard::is_within_board(0, -1));
  EXPECT_FALSE(SearchBoard::is_within_board(20, 0));
  EXPECT_FALSE(SearchBoard::is_within_board(0, 20));
}

TEST(SearchBoard, ToLinearIndex)
{
  EXPECT_EQ(SearchBoard::to_linear_index(0, 0), 0);
  EXPECT_EQ(SearchBoard::to_linear_index(3, 2), 43);
  EXPECT_EQ(SearchBoard::to_linear_index(19, 19), 399);
}

TEST(SearchBoard, SignCellConversions)
{
  EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::NONE), SearchBoard::Cell::EMPTY);
  EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::X), SearchBoard::Cell::X);
  EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::O), SearchBoard::Cell::O);
  EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::WALL), SearchBoard::Cell::WALL);

  EXPECT_EQ(SearchBoard::game_sign_from_cell(SearchBoard::Cell::EMPTY), Sign::NONE);
  EXPECT_EQ(SearchBoard::game_sign_from_cell(SearchBoard::Cell::X), Sign::X);
  EXPECT_EQ(SearchBoard::game_sign_from_cell(SearchBoard::Cell::O), Sign::O);
  EXPECT_EQ(SearchBoard::game_sign_from_cell(SearchBoard::Cell::WALL), Sign::WALL);
}

TEST(SearchBoard, OppositePlayerSign)
{
  EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::X), Sign::O);
  EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::O), Sign::X);
  EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::NONE), Sign::NONE);
  EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::WALL), Sign::NONE);
}

TEST(SearchBoard, LoadFromStateCopiesMetadata)
{
  const State state = MakeActiveState();
  SearchBoard board(state, Sign::X);

  EXPECT_EQ(board.my_sign(), Sign::X);
  EXPECT_EQ(board.opponent_sign(), Sign::O);
  EXPECT_EQ(board.move_number(), state.get_move_no());
  EXPECT_EQ(board.current_player(), state.get_current_player());
  EXPECT_EQ(board.game_status(), state.get_status());
  EXPECT_EQ(board.winner(), state.get_winner());
}

TEST(SearchBoard, LoadFromStateCopiesCells)
{
  const State state = MakeActiveState();
  SearchBoard board(state, Sign::X);

  EXPECT_EQ(board.get_cell(0, 0), SearchBoard::Cell::X);
  EXPECT_EQ(board.get_cell(2, 3), SearchBoard::Cell::O);
  EXPECT_EQ(board.get_cell(5, 5), SearchBoard::Cell::X);
  EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::EMPTY);
}

TEST(SearchBoard, EmptyBoardCellQueries)
{
  SearchBoard board;
  EXPECT_EQ(board.get_cell(0, 0), SearchBoard::Cell::EMPTY);
  EXPECT_TRUE(board.is_cell_empty(0, 0));
  EXPECT_FALSE(board.contains_stone(0, 0));
}

TEST(SearchBoard, OutOfBoundsReturnsWall)
{
  SearchBoard board;
  EXPECT_EQ(board.get_cell(-1, 0), SearchBoard::Cell::WALL);
  EXPECT_EQ(board.get_cell(0, -1), SearchBoard::Cell::WALL);
  EXPECT_EQ(board.get_cell(20, 0), SearchBoard::Cell::WALL);
  EXPECT_TRUE(board.is_wall_cell(-1, 0));
}

TEST(SearchBoard, SetCellAndQueryStone)
{
  const State state = MakeActiveState();
  SearchBoard board(state, Sign::X);

  board.set_cell(4, 4, SearchBoard::Cell::X);
  EXPECT_FALSE(board.is_cell_empty(4, 4));
  EXPECT_TRUE(board.contains_stone(4, 4));
  EXPECT_TRUE(board.contains_my_stone(4, 4));
  EXPECT_FALSE(board.contains_opponent_stone(4, 4));

  board.set_cell(4, 5, SearchBoard::Cell::O);
  EXPECT_FALSE(board.contains_my_stone(4, 5));
  EXPECT_TRUE(board.contains_opponent_stone(4, 5));

  board.set_cell(4, 6, SearchBoard::Cell::WALL);
  EXPECT_TRUE(board.is_wall_cell(4, 6));
  EXPECT_FALSE(board.contains_stone(4, 6));
}

TEST(SearchBoard, DetectsHorizontalFive)
{
  SearchBoard board;
  for (int x = 2; x <= 6; ++x)
    board.set_cell(x, 4, SearchBoard::Cell::X);

  EXPECT_TRUE(board.has_five_from(4, 4, Sign::X));
  EXPECT_TRUE(board.has_five_from(2, 4, Sign::X));
  EXPECT_TRUE(board.has_five_from(6, 4, Sign::X));
}

TEST(SearchBoard, DetectsVerticalFive)
{
  SearchBoard board;
  for (int y = 1; y <= 5; ++y)
    board.set_cell(8, y, SearchBoard::Cell::O);

  EXPECT_TRUE(board.has_five_from(8, 3, Sign::O));
}

TEST(SearchBoard, DetectsDiagonalFive)
{
  SearchBoard board;
  for (int d = 0; d < 5; ++d)
    board.set_cell(1 + d, 1 + d, SearchBoard::Cell::X);

  EXPECT_TRUE(board.has_five_from(3, 3, Sign::X));

  SearchBoard board2;
  for (int d = 0; d < 5; ++d)
    board2.set_cell(6 - d, 1 + d, SearchBoard::Cell::O);

  EXPECT_TRUE(board2.has_five_from(4, 3, Sign::O));
}

TEST(SearchBoard, BrokenLineIsNotFive)
{
  SearchBoard board;
  for (int x : {2, 3, 4, 6, 7})
    board.set_cell(x, 10, SearchBoard::Cell::X);

  EXPECT_FALSE(board.has_five_from(4, 10, Sign::X));
  EXPECT_FALSE(board.has_five_from(6, 10, Sign::X));
}

TEST(SearchBoard, WallBreaksLine)
{
  SearchBoard board;
  board.set_cell(2, 12, SearchBoard::Cell::X);
  board.set_cell(3, 12, SearchBoard::Cell::X);
  board.set_cell(4, 12, SearchBoard::Cell::WALL);
  board.set_cell(5, 12, SearchBoard::Cell::X);
  board.set_cell(6, 12, SearchBoard::Cell::X);

  EXPECT_FALSE(board.has_five_from(3, 12, Sign::X));
  EXPECT_FALSE(board.has_five_from(5, 12, Sign::X));
}

TEST(SearchBoard, HasFiveRejectsWrongSign)
{
  SearchBoard board;
  for (int x = 0; x < 5; ++x)
    board.set_cell(x, 0, SearchBoard::Cell::X);

  EXPECT_FALSE(board.has_five_from(2, 0, Sign::O));
  EXPECT_FALSE(board.has_five_from(2, 0, Sign::NONE));
  EXPECT_FALSE(board.has_five_from(-1, 0, Sign::X));
}

TEST(SearchBoard, ApplyMoveChangesStateAndUndoRestores)
{
  const State state = MakeActiveState();
  SearchBoard board(state, Sign::X);

  const int move_before = board.move_number();
  const Sign player_before = board.current_player();

  auto undo = board.apply_move(4, 4, Sign::X);

  EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::X);
  EXPECT_EQ(board.move_number(), move_before + 1);
  EXPECT_EQ(board.current_player(), Sign::O);

  board.undo_move(undo);

  EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::EMPTY);
  EXPECT_EQ(board.move_number(), move_before);
  EXPECT_EQ(board.current_player(), player_before);
}

TEST(SearchBoard, UndoInfoStoredCorrectly)
{
  const State state = MakeActiveState();
  SearchBoard board(state, Sign::X);

  auto undo = board.apply_move(4, 4, Sign::X);

  EXPECT_EQ(undo.x, 4);
  EXPECT_EQ(undo.y, 4);
  EXPECT_EQ(undo.previous_cell, SearchBoard::Cell::EMPTY);
  EXPECT_EQ(undo.previous_move_number, state.get_move_no());
  EXPECT_EQ(undo.previous_current_player, state.get_current_player());
  EXPECT_EQ(undo.previous_game_status, state.get_status());
  EXPECT_EQ(undo.previous_winner, state.get_winner());
}

TEST(SearchBoard, MultipleApplyAndUndoInOrder)
{
  const State state = MakeActiveState();
  SearchBoard board(state, Sign::X);

  auto undo1 = board.apply_move(4, 4, Sign::X);
  auto undo2 = board.apply_move(6, 6, Sign::O);

  EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::X);
  EXPECT_EQ(board.get_cell(6, 6), SearchBoard::Cell::O);
  EXPECT_EQ(board.move_number(), state.get_move_no() + 2);
  EXPECT_EQ(board.current_player(), Sign::X);

  board.undo_move(undo2);
  EXPECT_EQ(board.get_cell(6, 6), SearchBoard::Cell::EMPTY);
  EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::X);
  EXPECT_EQ(board.move_number(), state.get_move_no() + 1);
  EXPECT_EQ(board.current_player(), Sign::O);

  board.undo_move(undo1);
  EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::EMPTY);
  EXPECT_EQ(board.move_number(), state.get_move_no());
  EXPECT_EQ(board.current_player(), state.get_current_player());
  EXPECT_EQ(board.game_status(), state.get_status());
}

TEST(SearchBoard, WinningMoveEndsGame)
{
  State::Opts opts = DefaultOpts();
  State state(opts);

  state.process_move(Sign::X, 0, 0);
  state.process_move(Sign::O, 10, 10);
  state.process_move(Sign::X, 1, 0);
  state.process_move(Sign::O, 11, 10);
  state.process_move(Sign::X, 2, 0);
  state.process_move(Sign::O, 12, 10);
  state.process_move(Sign::X, 3, 0);
  state.process_move(Sign::O, 13, 10);

  SearchBoard board(state, Sign::X);
  auto undo = board.apply_move(4, 0, Sign::X);

  EXPECT_NE(board.game_status(), Status::ACTIVE);

  board.undo_move(undo);
  EXPECT_EQ(board.game_status(), Status::ACTIVE);
  EXPECT_EQ(board.current_player(), Sign::X);
}

TEST(SearchBoard, DrawOnMaxMoves)
{
  State::Opts opts = DefaultOpts();
  opts.max_moves = 2;
  State state(opts);
  state.process_move(Sign::X, 9, 9);

  SearchBoard board(state, Sign::O);
  auto undo = board.apply_move(10, 10, Sign::O);
  EXPECT_EQ(board.game_status(), Status::ENDED);

  board.undo_move(undo);
  EXPECT_EQ(board.game_status(), Status::ACTIVE);
}