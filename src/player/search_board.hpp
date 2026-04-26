#pragma once

#include "core/game.hpp"

#include <array>
#include <cstdint>

namespace ttt::my_player
{

  class SearchBoard
  {
  public:
    enum class Cell : std::uint8_t
    {
      EMPTY = 0,
      X = 1,
      O = 2,
      WALL = 3,
    };

    struct UndoInfo
    {
      int x = -1;
      int y = -1;
      Cell previous_cell = Cell::EMPTY;

      int previous_move_number = 0;
      int previous_max_move_count = 0;
      game::Sign previous_current_player = game::Sign::NONE;
      game::Status previous_game_status = game::Status::CREATED;
      game::Sign previous_winner = game::Sign::NONE;
    };

    static constexpr int kBoardWidth = 20;
    static constexpr int kBoardHeight = 20;
    static constexpr int kBoardCellCount = kBoardWidth * kBoardHeight;
    static constexpr int kWinLength = 5;

    SearchBoard() = default;
    SearchBoard(const game::State &state, game::Sign my_sign);

    void load_from_state(const game::State &state, game::Sign my_sign);

    static constexpr bool is_within_board(int x, int y)
    {
      return x >= 0 && x < kBoardWidth && y >= 0 && y < kBoardHeight;
    }

    static constexpr int to_linear_index(int x, int y)
    {
      return y * kBoardWidth + x;
    }

    Cell get_cell(int x, int y) const;
    void set_cell(int x, int y, Cell cell);

    bool is_cell_empty(int x, int y) const;
    bool is_wall_cell(int x, int y) const;
    bool contains_stone(int x, int y) const;
    bool contains_my_stone(int x, int y) const;
    bool contains_opponent_stone(int x, int y) const;
    bool has_five_from(int x, int y, game::Sign sign) const;

    game::Sign my_sign() const { return m_my_sign; }
    game::Sign opponent_sign() const { return m_opponent_sign; }
    game::Sign current_player() const { return m_current_player; }
    game::Sign winner() const { return m_winner; }
    game::Status game_status() const { return m_game_status; }
    int move_number() const { return m_move_number; }
    int max_move_count() const { return m_max_move_count; }

    UndoInfo apply_move(int x, int y, game::Sign sign);
    void undo_move(const UndoInfo &undo);

    static Cell cell_from_game_sign(game::Sign sign);
    static game::Sign game_sign_from_cell(Cell cell);
    static game::Sign opposite_player_sign(game::Sign sign);

  private:
    int count_stones_in_direction(int start_x, int start_y, int dx, int dy, Cell target_cell) const;
    void update_game_status_after_move(int x, int y, game::Sign sign);

    std::array<Cell, kBoardCellCount> m_cells{};

    game::Sign m_my_sign = game::Sign::NONE;
    game::Sign m_opponent_sign = game::Sign::NONE;
    game::Sign m_current_player = game::Sign::NONE;
    game::Sign m_winner = game::Sign::NONE;
    game::Status m_game_status = game::Status::CREATED;
    int m_move_number = 0;
    int m_max_move_count = 0;
  };

} // namespace ttt::my_player
