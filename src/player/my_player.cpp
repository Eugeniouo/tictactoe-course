#include "my_player.hpp"

#include <limits>

#include "board_utils.hpp"

namespace ttt::my_player
{

  namespace
  {
    struct DiffMove
    {
      int x = -1;
      int y = -1;
      Sign sign = Sign::NONE;
    };

    bool IsStone(Sign sign) noexcept
    {
      return sign == Sign::X || sign == Sign::O;
    }

    Point FindFirstEmptyNearCenter(const SearchBoard &board) noexcept
    {
      Point best = make_point(0, 0);
      int best_distance = std::numeric_limits<int>::max();

      for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
      {
        for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
        {
          if (!is_empty_cell(board, x, y))
          {
            continue;
          }

          const int distance = center_distance(x, y);

          if (distance < best_distance)
          {
            best_distance = distance;
            best = make_point(x, y);
          }
        }
      }

      return best;
    }
  }

  void MyPlayer::set_sign(Sign sign)
  {
    m_sign = sign;

    m_initialized = false;
    m_synced_move_number = -1;
  }

  const char *MyPlayer::get_name() const
  {
    return m_name;
  }

  Point MyPlayer::make_move(const State &state)
  {
    const Sign my_sign =
        m_sign != Sign::NONE ? m_sign : state.get_current_player();

    sync_from_state(state, my_sign);

    Point move = choose_best_move();

    if (!is_empty_cell(m_state.board(), move.x, move.y))
    {
      move = FindFirstEmptyNearCenter(m_state.board());
    }

    apply_own_move_to_cache(move, my_sign);

    return move;
  }

  void MyPlayer::reload_from_state(const State &state, Sign my_sign)
  {
    m_state.load_from_state(state, my_sign);

    m_initialized = true;
    m_synced_move_number = m_state.board().move_number();
  }

  void MyPlayer::sync_from_state(const State &state, Sign my_sign)
  {
    if (!m_initialized)
    {
      reload_from_state(state, my_sign);
      return;
    }

    const SearchBoard &board = m_state.board();

    const bool sign_changed = board.my_sign() != my_sign;
    const bool move_number_rewound =
        state.get_move_no() < m_synced_move_number;
    const bool move_limit_changed =
        state.get_opts().max_moves != board.max_move_count();

    if (sign_changed || move_number_rewound || move_limit_changed)
    {
      reload_from_state(state, my_sign);
      return;
    }

    DiffMove diff;
    int diff_count = 0;
    bool need_reload = false;

    for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
    {
      for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
      {
        const Sign state_sign = state.get_value(x, y);
        const SearchBoard::Cell state_cell =
            SearchBoard::cell_from_game_sign(state_sign);

        const SearchBoard::Cell cached_cell = board.get_cell(x, y);

        if (state_cell == cached_cell)
        {
          continue;
        }

        if (cached_cell == SearchBoard::Cell::EMPTY &&
            IsStone(state_sign))
        {
          diff.x = x;
          diff.y = y;
          diff.sign = state_sign;
          ++diff_count;

          if (diff_count > 1)
          {
            need_reload = true;
            break;
          }

          continue;
        }

        need_reload = true;
        break;
      }

      if (need_reload)
      {
        break;
      }
    }

    if (need_reload)
    {
      reload_from_state(state, my_sign);
      return;
    }

    if (diff_count == 1)
    {
      [[maybe_unused]] const SearchState::Undo undo =
          m_state.apply_move(diff.x, diff.y, diff.sign);
    }

    m_synced_move_number = m_state.board().move_number();
  }

  Point MyPlayer::choose_best_move()
  {
    const SearchBoard &board = m_state.board();

    const Sign my_sign =
        m_sign != Sign::NONE ? m_sign : board.current_player();

    const MoveList moves =
        m_move_generator.generate(
            m_state,
            my_sign,
            EngineConfig::kMaxQuietMoves);

    if (moves.empty())
    {
      return FindFirstEmptyNearCenter(board);
    }

    if (moves[0].tier == MoveTier::WIN ||
        moves[0].tier == MoveTier::BLOCK_WIN)
    {
      return moves[0].point;
    }

    const NegamaxResult search_result =
        m_negamax_searcher.find_best_move(m_state, my_sign, moves);

    if (search_result.found &&
        is_empty_cell(
            board,
            search_result.best_move.x,
            search_result.best_move.y))
    {
      return search_result.best_move;
    }

    return moves[0].point;
  }

  void MyPlayer::apply_own_move_to_cache(const Point &move, Sign sign)
  {
    if (!is_empty_cell(m_state.board(), move.x, move.y))
    {
      m_initialized = false;
      m_synced_move_number = -1;
      return;
    }

    [[maybe_unused]] const SearchState::Undo undo =
        m_state.apply_move(move.x, move.y, sign);
    m_synced_move_number = m_state.board().move_number();
  }

};
