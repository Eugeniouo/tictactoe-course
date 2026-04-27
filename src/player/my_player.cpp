/**
 * @brief оркестрация хода игрока поверх основных модулей
 *
 * @details
 * MyPlayer:
 *  - Получает состояние из движка
 *  - Строит SearchBoard
 *  - Сначала проверяет немедленные тактические ходы
 *  - Затем, если они не найдены, запускает negamax_search
 *  - Возвращает лучший найденный ход
 * ./build-wsl/tests/test_stats_vs_baseline - easy bot 100 игр
 * ./build-wsl/tests/test_with_baseline
 */

#include "my_player.hpp"
#include "instant_move_checker.hpp"
#include "move_generator.hpp"
#include "move_ordering.hpp"
#include "negamax_search.hpp"
#include "search_board.hpp"

namespace ttt::my_player
{

  void MyPlayer::set_sign(Sign sign)
  {
    m_sign = sign;
  }

  const char *MyPlayer::get_name() const
  {
    return m_name;
  }

  Point MyPlayer::make_move(const State &state)
  {
    SearchBoard board(state, m_sign);

    const InstantMoveResult instant = find_instant_move(board);
    if (SearchBoard::is_within_board(instant.move.x, instant.move.y))
    {
      return instant.move;
    }

    const SearchResult result = find_best_move(board, kSearchDepth);
    if (SearchBoard::is_within_board(result.best_move.x, result.best_move.y))
    {
      return result.best_move;
    }

    MoveList fallback_moves = generate_candidate_moves(board);
    order_candidate_moves(board, fallback_moves);

    if (!fallback_moves.empty())
    {
      return fallback_moves.front();
    }

    return Point{0, 0};
  }

}; // namespace ttt::my_player
