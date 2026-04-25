/**
 * @file test_search_board.cpp
 *
 * Проверяется рабочий функционал SearchBoard:
 * - статические helper-функции и преобразования типов;
 * - загрузка состояния доски из ttt::game::State;
 * - чтение и изменение клеток, EMPTY, X, O и WALL;
 * - is_cell_empty, is_wall_cell, contains_stone,
 *   contains_my_stone, contains_opponent_stone;
 * - применение хода apply_move;
 * - восстановление состояния через undo_move
 *
 * @test ctest --test-dir build -R "^test_search_board$" --output-on-failure
 */

#include "core/field.hpp"
#include "core/state.hpp"
#include "player/search_board.hpp"

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{

  using ttt::game::FieldBitmap;
  using ttt::game::IFieldInitializer;
  using ttt::game::Sign;
  using ttt::game::State;
  using ttt::game::Status;
  using ttt::my_player::SearchBoard;

  [[noreturn]] void fail(const std::string &message)
  {
    throw std::runtime_error(message);
  }

  void expect(bool condition, const std::string &message)
  {
    if (!condition)
    {
      fail(message);
    }
  }

  template <typename T, typename U>
  void expect_equal(const T &actual, const U &expected, const std::string &message)
  {
    if (!(actual == expected))
    {
      std::ostringstream stream;
      stream << message;
      fail(stream.str());
    }
  }

  State::Opts make_default_opts()
  {
    State::Opts opts{};
    opts.rows = SearchBoard::kBoardHeight;
    opts.cols = SearchBoard::kBoardWidth;
    opts.win_len = SearchBoard::kWinLength;
    opts.max_moves = 0;
    return opts;
  }

  class FixedWallsInitializer : public IFieldInitializer
  {
  public:
    void initialize(FieldBitmap &field) override
    {
      field.set(1, 1, Sign::WALL);
      field.set(10, 10, Sign::WALL);
      field.set(19, 0, Sign::WALL);
    }

    IFieldInitializer *clone() const override
    {
      return new FixedWallsInitializer(*this);
    }
  };

  State make_active_state_with_walls()
  {
    FixedWallsInitializer initializer;
    State state(make_default_opts(), &initializer);

    expect_equal(state.process_move(Sign::X, 0, 0), ttt::game::MoveResult::OK,
                 "Failed to place first X stone");
    expect_equal(state.process_move(Sign::O, 2, 3), ttt::game::MoveResult::OK,
                 "Failed to place first O stone");
    expect_equal(state.process_move(Sign::X, 5, 5), ttt::game::MoveResult::OK,
                 "Failed to place second X stone");
    expect_equal(state.process_move(Sign::O, 7, 8), ttt::game::MoveResult::OK,
                 "Failed to place second O stone");

    return state;
  }

  State make_ended_state_with_winner()
  {
    State::Opts opts = make_default_opts();
    opts.win_len = 3;

    State state(opts);

    expect_equal(state.process_move(Sign::X, 0, 0), ttt::game::MoveResult::OK,
                 "Failed to place winning sequence move 1");
    expect_equal(state.process_move(Sign::O, 5, 5), ttt::game::MoveResult::OK,
                 "Failed to place filler move 1");
    expect_equal(state.process_move(Sign::X, 1, 0), ttt::game::MoveResult::OK,
                 "Failed to place winning sequence move 2");
    expect_equal(state.process_move(Sign::O, 6, 6), ttt::game::MoveResult::OK,
                 "Failed to place filler move 2");
    expect_equal(state.process_move(Sign::X, 2, 0), ttt::game::MoveResult::OK,
                 "Failed to complete delayed win line");
    expect_equal(state.get_status(), Status::LAST_MOVE,
                 "Winning line should put the state into LAST_MOVE");
    expect_equal(state.process_move(Sign::O, 10, 10), ttt::game::MoveResult::WIN,
                 "Last move should finalize the delayed win");

    return state;
  }

  void test_static_helpers()
  {
    expect(SearchBoard::is_within_board(0, 0), "Origin must be on the board");
    expect(SearchBoard::is_within_board(19, 19),
           "Bottom-right corner must be on the board");
    expect(!SearchBoard::is_within_board(-1, 0),
           "Negative x must be outside the board");
    expect(!SearchBoard::is_within_board(0, -1),
           "Negative y must be outside the board");
    expect(!SearchBoard::is_within_board(20, 0),
           "x equal to width must be outside the board");
    expect(!SearchBoard::is_within_board(0, 20),
           "y equal to height must be outside the board");

    expect_equal(SearchBoard::to_linear_index(0, 0), 0,
                 "Linear index for origin is wrong");
    expect_equal(SearchBoard::to_linear_index(3, 2), 43,
                 "Linear index for (3, 2) is wrong");
    expect_equal(SearchBoard::to_linear_index(19, 19), 399,
                 "Linear index for bottom-right corner is wrong");

    expect_equal(SearchBoard::cell_from_game_sign(Sign::NONE), SearchBoard::Cell::EMPTY,
                 "NONE sign must map to EMPTY cell");
    expect_equal(SearchBoard::cell_from_game_sign(Sign::X), SearchBoard::Cell::X,
                 "X sign must map to X cell");
    expect_equal(SearchBoard::cell_from_game_sign(Sign::O), SearchBoard::Cell::O,
                 "O sign must map to O cell");
    expect_equal(SearchBoard::cell_from_game_sign(Sign::WALL), SearchBoard::Cell::WALL,
                 "WALL sign must map to WALL cell");

    expect_equal(SearchBoard::game_sign_from_cell(SearchBoard::Cell::EMPTY), Sign::NONE,
                 "EMPTY cell must map to NONE sign");
    expect_equal(SearchBoard::game_sign_from_cell(SearchBoard::Cell::X), Sign::X,
                 "X cell must map to X sign");
    expect_equal(SearchBoard::game_sign_from_cell(SearchBoard::Cell::O), Sign::O,
                 "O cell must map to O sign");
    expect_equal(SearchBoard::game_sign_from_cell(SearchBoard::Cell::WALL), Sign::WALL,
                 "WALL cell must map to WALL sign");

    expect_equal(SearchBoard::opposite_player_sign(Sign::X), Sign::O,
                 "Opposite sign for X is wrong");
    expect_equal(SearchBoard::opposite_player_sign(Sign::O), Sign::X,
                 "Opposite sign for O is wrong");
    expect_equal(SearchBoard::opposite_player_sign(Sign::NONE), Sign::NONE,
                 "Opposite sign for NONE must stay NONE");
    expect_equal(SearchBoard::opposite_player_sign(Sign::WALL), Sign::NONE,
                 "Opposite sign for WALL must be NONE");
  }

  void test_loading_from_state()
  {
    const State active_state = make_active_state_with_walls();
    SearchBoard board(active_state, Sign::X);

    expect_equal(board.my_sign(), Sign::X, "my_sign must be copied from constructor input");
    expect_equal(board.opponent_sign(), Sign::O,
                 "opponent_sign must be derived from my_sign");
    expect_equal(board.current_player(), active_state.get_current_player(),
                 "current_player must be copied from active state");
    expect_equal(board.winner(), active_state.get_winner(),
                 "winner must be copied from active state");
    expect_equal(board.game_status(), active_state.get_status(),
                 "game_status must be copied from active state");
    expect_equal(board.move_number(), active_state.get_move_no(),
                 "move_number must be copied from active state");

    expect_equal(board.get_cell(0, 0), SearchBoard::Cell::X,
                 "Active state X stone was not loaded");
    expect_equal(board.get_cell(2, 3), SearchBoard::Cell::O,
                 "Active state O stone was not loaded");
    expect_equal(board.get_cell(1, 1), SearchBoard::Cell::WALL,
                 "Wall at (1, 1) was not loaded");
    expect_equal(board.get_cell(10, 10), SearchBoard::Cell::WALL,
                 "Wall at (10, 10) was not loaded");
    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::EMPTY,
                 "Empty cell should stay empty after load");

    const State ended_state = make_ended_state_with_winner();
    board.load_from_state(ended_state, Sign::O);

    expect_equal(board.my_sign(), Sign::O, "my_sign must update on reload");
    expect_equal(board.opponent_sign(), Sign::X,
                 "opponent_sign must update on reload");
    expect_equal(board.current_player(), ended_state.get_current_player(),
                 "current_player must be copied from ended state");
    expect_equal(board.winner(), ended_state.get_winner(),
                 "winner must be copied from ended state");
    expect_equal(board.game_status(), ended_state.get_status(),
                 "game_status must be copied from ended state");
    expect_equal(board.move_number(), ended_state.get_move_no(),
                 "move_number must be copied from ended state");
    expect_equal(board.get_cell(0, 0), SearchBoard::Cell::X,
                 "Reloaded board lost X stone from ended state");
    expect_equal(board.get_cell(5, 5), SearchBoard::Cell::O,
                 "Reloaded board lost O stone from ended state");
  }

  void test_cell_queries_and_setters()
  {
    SearchBoard board;

    expect_equal(board.get_cell(0, 0), SearchBoard::Cell::EMPTY,
                 "Default board must start empty");
    expect_equal(board.get_cell(-1, 0), SearchBoard::Cell::WALL,
                 "Cells outside the board must behave like walls");
    expect(board.is_cell_empty(0, 0), "Empty cell was not recognized");
    expect(board.is_wall_cell(-1, 0), "Out-of-bounds cell must be treated as wall");
    expect(!board.contains_stone(0, 0), "Empty cell must not contain a stone");

    const State active_state = make_active_state_with_walls();
    board.load_from_state(active_state, Sign::X);

    board.set_cell(4, 4, SearchBoard::Cell::X);
    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::X,
                 "set_cell did not store X");
    expect(!board.is_cell_empty(4, 4), "Cell with X must not be empty");
    expect(board.contains_stone(4, 4), "Cell with X must contain a stone");
    expect(board.contains_my_stone(4, 4), "Cell with my sign must be detected");
    expect(!board.contains_opponent_stone(4, 4),
           "Cell with my sign must not be marked as opponent stone");

    board.set_cell(4, 5, SearchBoard::Cell::O);
    expect(board.contains_stone(4, 5), "Cell with O must contain a stone");
    expect(!board.contains_my_stone(4, 5),
           "Cell with opponent sign must not be marked as my stone");
    expect(board.contains_opponent_stone(4, 5),
           "Cell with opponent sign must be detected");

    board.set_cell(4, 6, SearchBoard::Cell::WALL);
    expect(board.is_wall_cell(4, 6), "Explicit wall cell must be detected");
    expect(!board.contains_stone(4, 6), "Wall must not be treated as a stone");
  }

  void test_apply_move_and_single_undo()
  {
    const State active_state = make_active_state_with_walls();
    SearchBoard board(active_state, Sign::X);

    const SearchBoard::UndoInfo undo = board.apply_move(4, 4, Sign::X);

    expect_equal(undo.x, 4, "UndoInfo must keep x coordinate");
    expect_equal(undo.y, 4, "UndoInfo must keep y coordinate");
    expect_equal(undo.previous_cell, SearchBoard::Cell::EMPTY,
                 "UndoInfo must capture previous empty cell");
    expect_equal(undo.previous_move_number, active_state.get_move_no(),
                 "UndoInfo must capture previous move number");
    expect_equal(undo.previous_current_player, active_state.get_current_player(),
                 "UndoInfo must capture previous current player");
    expect_equal(undo.previous_game_status, active_state.get_status(),
                 "UndoInfo must capture previous game status");
    expect_equal(undo.previous_winner, active_state.get_winner(),
                 "UndoInfo must capture previous winner");

    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::X,
                 "apply_move must place the requested sign");
    expect_equal(board.move_number(), active_state.get_move_no() + 1,
                 "apply_move must increment move number");
    expect_equal(board.current_player(), Sign::O,
                 "apply_move must switch current player to the opposite sign");
    expect_equal(board.game_status(), active_state.get_status(),
                 "apply_move must not change game_status in current implementation");
    expect_equal(board.winner(), active_state.get_winner(),
                 "apply_move must not change winner in current implementation");

    board.undo_move(undo);

    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::EMPTY,
                 "undo_move must restore previous cell value");
    expect_equal(board.move_number(), active_state.get_move_no(),
                 "undo_move must restore move number");
    expect_equal(board.current_player(), active_state.get_current_player(),
                 "undo_move must restore current player");
    expect_equal(board.game_status(), active_state.get_status(),
                 "undo_move must restore game status");
    expect_equal(board.winner(), active_state.get_winner(),
                 "undo_move must restore winner");
  }

  void test_multiple_undos_restore_original_state()
  {
    const State active_state = make_active_state_with_walls();
    SearchBoard board(active_state, Sign::O);

    const SearchBoard::UndoInfo first_undo = board.apply_move(4, 4, Sign::X);
    const SearchBoard::UndoInfo second_undo = board.apply_move(6, 6, Sign::O);

    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::X,
                 "First applied move must stay on board before undo");
    expect_equal(board.get_cell(6, 6), SearchBoard::Cell::O,
                 "Second applied move must stay on board before undo");
    expect_equal(board.move_number(), active_state.get_move_no() + 2,
                 "Two applied moves must increment move number twice");
    expect_equal(board.current_player(), Sign::X,
                 "Current player after X then O must become X");

    board.undo_move(second_undo);
    expect_equal(board.get_cell(6, 6), SearchBoard::Cell::EMPTY,
                 "Undoing second move must clear second cell");
    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::X,
                 "Undoing second move must not affect first move");
    expect_equal(board.move_number(), active_state.get_move_no() + 1,
                 "Undoing second move must decrement move number once");
    expect_equal(board.current_player(), Sign::O,
                 "Undoing second move must restore previous current player");

    board.undo_move(first_undo);
    expect_equal(board.get_cell(4, 4), SearchBoard::Cell::EMPTY,
                 "Undoing first move must clear first cell");
    expect_equal(board.get_cell(0, 0), SearchBoard::Cell::X,
                 "Undo sequence must preserve original X stones");
    expect_equal(board.get_cell(2, 3), SearchBoard::Cell::O,
                 "Undo sequence must preserve original O stones");
    expect_equal(board.get_cell(1, 1), SearchBoard::Cell::WALL,
                 "Undo sequence must preserve walls");
    expect_equal(board.move_number(), active_state.get_move_no(),
                 "Undo sequence must restore original move number");
    expect_equal(board.current_player(), active_state.get_current_player(),
                 "Undo sequence must restore original current player");
    expect_equal(board.game_status(), active_state.get_status(),
                 "Undo sequence must restore original game status");
    expect_equal(board.winner(), active_state.get_winner(),
                 "Undo sequence must restore original winner");
  }

} // namespace

int main()
{
  try
  {
    test_static_helpers();
    test_loading_from_state();
    test_cell_queries_and_setters();
    test_apply_move_and_single_undo();
    test_multiple_undos_restore_original_state();
  }
  catch (const std::exception &error)
  {
    std::cerr << "test_search_board failed: " << error.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "test_search_board passed\n";
  return EXIT_SUCCESS;
}
