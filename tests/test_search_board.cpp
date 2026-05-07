/**
 * @test ctest --test-dir build -R "^test_search_board$" --output-on-failure
 */

#include "core/state.hpp"
#include "player/search_board.hpp"

#include <gtest/gtest.h>

using ttt::game::Sign;
using ttt::game::State;
using ttt::game::Status;
using ttt::my_player::SearchBoard;

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

    State make_active_state()
    {
        State state(default_opts());
        state.process_move(Sign::X, 0, 0);
        state.process_move(Sign::O, 2, 3);
        state.process_move(Sign::X, 5, 5);
        state.process_move(Sign::O, 7, 8);
        return state;
    }
}

TEST(SearchBoard, ChecksBoardBorders)
{
    EXPECT_TRUE(SearchBoard::is_within_board(0, 0));
    EXPECT_TRUE(SearchBoard::is_within_board(19, 19));
    EXPECT_FALSE(SearchBoard::is_within_board(-1, 0));
    EXPECT_FALSE(SearchBoard::is_within_board(0, -1));
    EXPECT_FALSE(SearchBoard::is_within_board(20, 0));
    EXPECT_FALSE(SearchBoard::is_within_board(0, 20));
}

TEST(SearchBoard, ConvertsCoordinatesToPaddedIndex)
{
    EXPECT_EQ(SearchBoard::to_index(0, 0), 116);
    EXPECT_EQ(SearchBoard::to_index(3, 2), 175);
    EXPECT_EQ(SearchBoard::to_index(19, 19), 667);
}

TEST(SearchBoard, ConvertsSignsAndOppositeSide)
{
    EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::NONE), SearchBoard::Cell::EMPTY);
    EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::X), SearchBoard::Cell::X);
    EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::O), SearchBoard::Cell::O);
    EXPECT_EQ(SearchBoard::cell_from_game_sign(Sign::WALL), SearchBoard::Cell::WALL);

    EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::X), Sign::O);
    EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::O), Sign::X);
    EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::NONE), Sign::NONE);
    EXPECT_EQ(SearchBoard::opposite_player_sign(Sign::WALL), Sign::NONE);
}

TEST(SearchBoard, LoadsCellsAndMetadataFromCoreState)
{
    const State state = make_active_state();

    SearchBoard board;
    board.load_from_state(state, Sign::X);

    EXPECT_EQ(board.my_sign(), Sign::X);
    EXPECT_EQ(board.move_number(), state.get_move_no());
    EXPECT_EQ(board.current_player(), state.get_current_player());
    EXPECT_EQ(board.game_status(), state.get_status());
    EXPECT_EQ(board.winner(), state.get_winner());
    EXPECT_EQ(board.stone_count(), 4);

    EXPECT_EQ(board.get_cell(0, 0), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(2, 3), SearchBoard::Cell::O);
    EXPECT_EQ(board.get_cell(5, 5), SearchBoard::Cell::X);
    EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::EMPTY);
}

TEST(SearchBoard, ReturnsWallOutsidePlayableArea)
{
    SearchBoard board;

    EXPECT_EQ(board.get_cell(0, 0), SearchBoard::Cell::EMPTY);
    EXPECT_EQ(board.get_cell(-1, 0), SearchBoard::Cell::WALL);
    EXPECT_EQ(board.get_cell(0, -1), SearchBoard::Cell::WALL);
    EXPECT_EQ(board.get_cell(20, 0), SearchBoard::Cell::WALL);
    EXPECT_EQ(board.get_cell(0, 20), SearchBoard::Cell::WALL);
}

TEST(SearchBoard, ApplyMoveChangesStateAndUndoRestoresIt)
{
    const State state = make_active_state();

    SearchBoard board;
    board.load_from_state(state, Sign::X);

    const int move_number_before = board.move_number();
    const int stone_count_before = board.stone_count();
    const Sign player_before = board.current_player();
    const Status status_before = board.game_status();
    const std::uint64_t hash_before = board.hash();

    const SearchBoard::UndoInfo undo = board.apply_move(4, 4, Sign::X);

    EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::X);
    EXPECT_EQ(board.move_number(), move_number_before + 1);
    EXPECT_EQ(board.stone_count(), stone_count_before + 1);
    EXPECT_EQ(board.current_player(), Sign::O);
    EXPECT_NE(board.hash(), hash_before);
    EXPECT_EQ(board.stone_index_at(stone_count_before), SearchBoard::to_index(4, 4));

    board.undo_move(undo);

    EXPECT_EQ(board.get_cell(4, 4), SearchBoard::Cell::EMPTY);
    EXPECT_EQ(board.move_number(), move_number_before);
    EXPECT_EQ(board.stone_count(), stone_count_before);
    EXPECT_EQ(board.current_player(), player_before);
    EXPECT_EQ(board.game_status(), status_before);
    EXPECT_EQ(board.hash(), hash_before);
}

TEST(SearchBoard, WinningOddMoveGivesOpponentLastReply)
{
    State state(default_opts());
    state.process_move(Sign::X, 0, 0);
    state.process_move(Sign::O, 10, 10);
    state.process_move(Sign::X, 1, 0);
    state.process_move(Sign::O, 11, 10);
    state.process_move(Sign::X, 2, 0);
    state.process_move(Sign::O, 12, 10);
    state.process_move(Sign::X, 3, 0);
    state.process_move(Sign::O, 13, 10);

    SearchBoard board;
    board.load_from_state(state, Sign::X);

    const SearchBoard::UndoInfo undo = board.apply_move(4, 0, Sign::X);

    EXPECT_EQ(board.game_status(), Status::LAST_MOVE);
    EXPECT_EQ(board.winner(), Sign::NONE);
    EXPECT_EQ(board.current_player(), Sign::O);

    board.undo_move(undo);

    EXPECT_EQ(board.game_status(), Status::ACTIVE);
    EXPECT_EQ(board.current_player(), Sign::X);
    EXPECT_EQ(board.winner(), Sign::NONE);
}

TEST(SearchBoard, MaxMoveLimitEndsAsDraw)
{
    State::Opts opts = default_opts();
    opts.max_moves = 2;

    State state(opts);
    state.process_move(Sign::X, 9, 9);

    SearchBoard board;
    board.load_from_state(state, Sign::O);

    const SearchBoard::UndoInfo undo = board.apply_move(10, 10, Sign::O);

    EXPECT_EQ(board.game_status(), Status::ENDED);
    EXPECT_EQ(board.winner(), Sign::NONE);

    board.undo_move(undo);

    EXPECT_EQ(board.game_status(), Status::ACTIVE);
    EXPECT_EQ(board.current_player(), Sign::O);
}
