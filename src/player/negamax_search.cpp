#include "negamax_search.hpp"
#include "move_generator.hpp"
#include <limits>

namespace ttt::my_player
{
    namespace
    {
        constexpr Score kSearchInfinity = std::numeric_limits<Score>::max() / 4;

        struct SearchContext
        {
            std::uint64_t visited_nodes = 0;
        };

        bool is_player_sign(game::Sign sign)
        {
            return sign == game::Sign::X || sign == game::Sign::O;
        }

        /**
         * @brief Рекурсивно оценивает позицию алгоритмом negamax с alpha-beta отсечением.
         *
         * @param board Текущая поисковая позиция.
         * @param depth Оставшаяся глубина поиска.
         * @param alpha Нижняя граница уже гарантированного результата для текущей стороны.
         * @param beta Верхняя граница допустимого результата для текущей стороны.
         * @param ctx Счетчики и служебный контекст текущего поиска.
         * @return Score Оценка позиции с точки зрения игрока, которому сейчас ходить.
         *
         * @details
         * Алгоритм работает по схеме:
         * - если достигнут лист или terminal-позиция, вернуть статическую оценку
         * - сгенерировать кандидатные ходы
         * - для каждого хода сделать apply_move
         * - рекурсивно посчитать ответ соперника
         * - инвертировать знак результата
         * - обновить лучший score и окно alpha/beta
         * - при alpha >= beta прекратить просмотр оставшихся ходов.
         */
        Score negamax(SearchBoard &board, int depth, Score alpha, Score beta, SearchContext &ctx)
        {
            ++ctx.visited_nodes;

            const game::Sign player = board.current_player();
            if (!is_player_sign(player))
            {
                return 0;
            }

            if (depth <= 0 || board.game_status() == game::Status::ENDED)
            {
                return evaluate_position(board, player);
            }

            const MoveList moves = generate_candidate_moves(board);
            if (moves.empty())
            {
                return evaluate_position(board, player);
            }

            Score best_score = -kSearchInfinity;

            for (const game::Point &move : moves)
            {
                const SearchBoard::UndoInfo undo = board.apply_move(move.x, move.y, player);
                const Score score = -negamax(board, depth - 1, -beta, -alpha, ctx);
                board.undo_move(undo);

                if (score > best_score)
                    best_score = score;

                if (score > alpha)
                    alpha = score;

                if (alpha >= beta)
                    break;
            }

            return best_score;
        }
    }

    SearchResult find_best_move(SearchBoard board, int depth)
    {
        SearchResult result{};

        const game::Sign player = board.current_player();
        if (!is_player_sign(player))
        {
            return result;
        }

        if (depth <= 0 || board.game_status() == game::Status::ENDED)
        {
            result.best_score = evaluate_position(board, player);
            result.visited_nodes = 1;
            return result;
        }

        const MoveList moves = generate_candidate_moves(board);
        if (moves.empty())
        {
            return result;
        }

        SearchContext ctx{};
        result.best_move = moves.front();
        Score alpha = -kSearchInfinity;

        for (const game::Point &move : moves)
        {
            const SearchBoard::UndoInfo undo = board.apply_move(move.x, move.y, player);
            const Score score = -negamax(board, depth - 1, -kSearchInfinity, kSearchInfinity, ctx);
            board.undo_move(undo);

            if (score > alpha)
            {
                alpha = score;
                result.best_move = move;
            }
        }

        result.best_score = alpha;
        result.visited_nodes = ctx.visited_nodes;
        return result;
    }

} // namespace ttt::my_player
