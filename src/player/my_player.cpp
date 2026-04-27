/**
 * @brief оркестрация хода игрока поверх основных модулей
 * 
 * @details
 * MyPlayer:
 *  - Получает состояние из движка
 *  - Строит SearchBoard
 *  - Запускает negamax_search
 *  - Возвращает лучший найденный ход
 */

#include "my_player.hpp"
#include "move_generator.hpp"
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

    const SearchResult result = find_best_move(board, kSearchDepth);
    if (SearchBoard::is_within_board(result.best_move.x, result.best_move.y))
    {
      return result.best_move;
    }

    const MoveList fallback_moves = generate_candidate_moves(board);

    if (!fallback_moves.empty())
    {
      return fallback_moves.front();
    }

    return Point{0, 0};
  }

}; // namespace ttt::my_player
