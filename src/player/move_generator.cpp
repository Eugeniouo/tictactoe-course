#include "move_generator.hpp"
#include <array>
#include <limits>
#include <cassert>

namespace ttt::my_player
{
    namespace
    {

        constexpr int kCandidatRadius = 2;

        bool is_candidate_cell(const SearchBoard &board, int x, int y)
        {
            return SearchBoard::is_within_board(x, y) && board.is_cell_empty(x, y);
        }

        /**
         * @brief Находит дебютный fallback-ход, ближайший к центру доски.
         *
         * @param board Текущее внутреннее представление позиции.
         * @return game::Point Координаты пустой клетки, ближайшей к геометрическому
         * центру поля.
         *
         * @details
         * Функция используется в двух случаях:
         * - на доске ещё нет ни одного камня;
         * - после локальной генерации не найдено ни одного кандидатного хода.
         *
         * Для выбора клетки вычисляется квадрат расстояния до геометрического центра
         * доски. Чтобы избежать работы с дробными числами для доски чётного размера,
         * сравнение выполняется в удвоенных координатах.
         */
        game::Point find_nearest_center_move(const SearchBoard &board)
        {
            const int cx2 = SearchBoard::kBoardWidth - 1;
            const int cy2 = SearchBoard::kBoardHeight - 1;

            game::Point best{-1, -1};
            int best_distance = std::numeric_limits<int>::max(); // максимальный возможнный int

            for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
            {
                for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
                {
                    if (!board.is_cell_empty(x, y))
                    {
                        continue;
                    }

                    const int dx = 2 * x - cx2;
                    const int dy = 2 * y - cy2;
                    const int distance = (dx * dx) + (dy * dy);

                    if (distance < best_distance)
                    {
                        best_distance = distance;
                        best = {x, y};
                    }
                }
            }

            assert(best.x != -1 && best.y != -1);
            return best;
        }
    }

    /**
     * @brief Генерирует список кандидатных ходов для поискового алгоритма.
     *
     * @param board Текущее внутреннее представление позиции.
     * @return MoveList Список пустых клеток, расположенных в квадрате радиуса 2
     * вокруг хотя бы одного уже стоящего на доске камня.
     *
     * @details
     * Функция сокращает пространство перебора перед negamax/alpha-beta:
     * вместо просмотра всех пустых клеток на поле рассматриваются только
     * локально значимые ходы рядом с уже существующими X и O.
     *
     * Алгоритм работы:
     * - проходит по всей доске
     * - находит все клетки, содержащие камни X или O
     * - для каждой такой клетки просматривает квадрат радиуса 2
     * - добавляет в результат только пустые клетки
     * - устраняет дубликаты, возникающие из-за пересечения окрестностей
     *
     * @note
     * Если на доске ещё нет камней или локальные кандидаты не найдены,
     * функция возвращает один fallback-ход: пустую клетку, ближайшую
     * к геометрическому центру доски.
     */
    MoveList generate_candidate_moves(const SearchBoard &board)
    {
        MoveList moves;
        std::array<bool, SearchBoard::kBoardCellCount> already_added{};
        bool has_stone = false;

        for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
        {
            for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
            {
                if (!board.contains_stone(x, y))
                {
                    continue;
                }

                has_stone = true;

                for (int dy = -kCandidatRadius; dy <= kCandidatRadius; ++dy)
                {
                    for (int dx = -kCandidatRadius; dx <= kCandidatRadius; ++dx)
                    {
                        const int nx = x + dx;
                        const int ny = y + dy;

                        if (!is_candidate_cell(board, nx, ny))
                        {
                            continue;
                        }

                        const int index = SearchBoard::to_linear_index(nx, ny);
                        if (already_added[index])
                        {
                            continue;
                        }

                        already_added[index] = true;
                        moves.push_back({nx, ny});
                    }
                }
            }
        }

        if (!has_stone || moves.empty())
        {
            moves.push_back(find_nearest_center_move(board));
        }

        return moves;
    }
} // namespace ttt::my_player