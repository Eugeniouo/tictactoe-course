/**
 * @test ctest --test-dir build -R "^test_move_generator$" --output-on-failure
 */

#include "core/field.hpp"
#include "core/state.hpp"
#include "player/move_generator.hpp"

#include <gtest/gtest.h>
#include <algorithm>

using ttt::game::Point;
using ttt::game::Sign;
using ttt::game::State;
using ttt::my_player::generate_candidate_moves;
using ttt::my_player::MoveList;
using ttt::my_player::SearchBoard;

namespace
{
    State::Opts defaultOpts()
    {
        State::Opts opts{};
        opts.rows = SearchBoard::kBoardHeight;
        opts.cols = SearchBoard::kBoardWidth;
        opts.win_len = SearchBoard::kWinLength;
        opts.max_moves = 0;
        return opts;
    }

    /**
     * @brief Проверяет, есть ли ход с заданными координатами в списке ходов.
     *
     * Проходит по списку ходов и возвращает true, если хотя бы один ход
     * имеет координаты, равные заданным значениям x и y.
     *
     * @param moves Список ходов, в котором выполняется поиск. Вектор
     * @param x Координата X искомого хода
     * @param y Координата Y искомого хода
     *
     * @return true, если ход с координатами (x, y) найден
     *         иначе false.
     */
    bool ContainsMove(const MoveList &moves, int x, int y)
    {
        return std::any_of(moves.begin(), moves.end(), [x, y](const Point &move)
                           { return move.x == x && move.y == y; });
    }

}

TEST(MoveGenerator, EmptyBoardReturnsSingleCentralMove)
{
    SearchBoard board;

    const MoveList moves = generate_candidate_moves(board);

    ASSERT_EQ(moves.size(), 1u);

    const Point move = moves.front();
    EXPECT_TRUE(
        (move.x == 9 || move.x == 10) &&
        (move.y == 9 || move.y == 10));
}

TEST(MoveGenerator, SingleStoneInCenterGenerates24Candidates)
{
    SearchBoard board;
    board.set_cell(10, 10, SearchBoard::Cell::X);

    const MoveList moves = generate_candidate_moves(board);

    EXPECT_EQ(moves.size(), 24u);
    EXPECT_FALSE(ContainsMove(moves, 10, 10));
    EXPECT_TRUE(ContainsMove(moves, 8, 8));
    EXPECT_TRUE(ContainsMove(moves, 12, 12));
    EXPECT_TRUE(ContainsMove(moves, 10, 8));
    EXPECT_TRUE(ContainsMove(moves, 12, 10));
}

TEST(MoveGenerator, SingleStoneInCornerClipsToBoard)
{
    SearchBoard board;
    board.set_cell(0, 0, SearchBoard::Cell::X);

    const MoveList moves = generate_candidate_moves(board);

    EXPECT_EQ(moves.size(), 8u);
    EXPECT_FALSE(ContainsMove(moves, 0, 0));
    EXPECT_TRUE(ContainsMove(moves, 0, 1));
    EXPECT_TRUE(ContainsMove(moves, 1, 0));
    EXPECT_TRUE(ContainsMove(moves, 2, 2));
}

TEST(MoveGenerator, DoesNotIncludeOccupiedCells)
{
    SearchBoard board;
    board.set_cell(10, 10, SearchBoard::Cell::X);
    board.set_cell(11, 10, SearchBoard::Cell::O);

    const MoveList moves = generate_candidate_moves(board);

    EXPECT_FALSE(ContainsMove(moves, 10, 10));
    EXPECT_FALSE(ContainsMove(moves, 11, 10));
}

TEST(MoveGenerator, DoesNotIncludeWalls)
{
    SearchBoard board;
    board.set_cell(10, 10, SearchBoard::Cell::X);
    board.set_cell(9, 9, SearchBoard::Cell::WALL);

    const MoveList moves = generate_candidate_moves(board);

    EXPECT_FALSE(ContainsMove(moves, 9, 9));
    EXPECT_TRUE(ContainsMove(moves, 8, 8));
}

TEST(MoveGenerator, DoesNotDuplicateOverlappingNeighborhoods)
{
    SearchBoard board;
    board.set_cell(10, 10, SearchBoard::Cell::X);
    board.set_cell(11, 10, SearchBoard::Cell::O);

    const MoveList moves = generate_candidate_moves(board);

    int count = 0;
    for (const Point &move : moves)
    {
        if (move.x == 9 && move.y == 10)
        {
            ++count;
        }
    }

    EXPECT_EQ(count, 1);
}

TEST(MoveGenerator, TwoDistantStonesGenerateUnionOfNeighborhoods)
{
    SearchBoard board;
    board.set_cell(0, 0, SearchBoard::Cell::X);
    board.set_cell(19, 19, SearchBoard::Cell::O);

    const MoveList moves = generate_candidate_moves(board);

    EXPECT_TRUE(ContainsMove(moves, 1, 0));
    EXPECT_TRUE(ContainsMove(moves, 18, 19));
    EXPECT_FALSE(ContainsMove(moves, 0, 0));
    EXPECT_FALSE(ContainsMove(moves, 19, 19));
}
