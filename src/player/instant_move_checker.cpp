#include "instant_move_checker.hpp"
#include "move_generator.hpp"

#include <optional>

namespace ttt::my_player
{
    namespace
    {
        enum class TacticalOutcome
        {
            NONE,
            WIN,
            DRAW,
            FORCE_LAST_MOVE
        };

        /**
         * @brief Классифицирует немедленный результат уже примененного хода.
         *
         * @param board Позиция после временного применения хода.
         * @param mover Игрок, который только что сделал этот ход.
         * @return TacticalOutcome Тип найденного тактического результата.
         *
         * @details
         * Функция использует обновленное metadata-состояние позиции, чтобы
         * корректно учитывать специальное правило с `LAST_MOVE`.
         * Это важно, потому что "на доске есть five" и "партия уже terminal"
         * в этом проекте не всегда одно и то же.
         */
        TacticalOutcome classify_tactical_outcome(const SearchBoard &board, game::Sign mover)
        {
            switch (board.game_status())
            {
            case game::Status::LAST_MOVE:
                return TacticalOutcome::FORCE_LAST_MOVE;

            case game::Status::ENDED:
                if (board.winner() == mover)
                {
                    return TacticalOutcome::WIN;
                }

                if (board.winner() == game::Sign::NONE)
                {
                    return TacticalOutcome::DRAW;
                }

                return TacticalOutcome::NONE;

            default:
                return TacticalOutcome::NONE;
            }
        }

        /**
         * @brief Проверяет, есть ли у указанного игрока немедленный решающий ответ.
         *
         * @param board Текущая позиция, в которой проверяются ответы.
         * @param player Игрок, для которого ищутся решающие ходы.
         * @return true Если существует хотя бы один немедленно решающий ход.
         * @return false Если таких ходов нет.
         *
         * @details
         * Функция перебирает кандидатные ходы, временно применяет каждый из них,
         * классифицирует результат и затем откатывает позицию через `undo_move`.
         */
        bool has_decisive_reply(SearchBoard &board, game::Sign player)
        {
            for (const game::Point &move : generate_candidate_moves(board))
            {
                const SearchBoard::UndoInfo undo = board.apply_move(move.x, move.y, player);
                const TacticalOutcome outcome = classify_tactical_outcome(board, player);
                board.undo_move(undo);

                if (outcome != TacticalOutcome::NONE)
                {
                    return true;
                }
            }

            return false;
        }
    } // namespace

    /**
     * @brief Ищет немедленный тактический ход до запуска основного поиска.
     *
     * @param board Текущая поисковая позиция.
     * @return InstantMoveResult Найденный ход и причина его выбора.
     *
     * @details
     * Функция сначала ищет собственный решающий ход текущего игрока.
     * Если такого хода нет, она проверяет, есть ли у соперника немедленный
     * решающий ответ, и пытается найти ход, который убирает все такие ответы.
     *
     * Позиция передается по значению, чтобы локальные тактические проверки
     * не могли повредить исходное состояние, с которым продолжит работать
     * остальная логика выбора хода.
     */
    InstantMoveResult find_instant_move(SearchBoard &board)
    {
        const game::Sign self = board.current_player();
        if (self != game::Sign::X && self != game::Sign::O)
        {
            return {};
        }

        if (board.game_status() == game::Status::ENDED)
        {
            return {};
        }

        const game::Sign opp = SearchBoard::opposite_player_sign(self);
        const MoveList root_moves = generate_candidate_moves(board);

        std::optional<game::Point> draw_move;
        std::optional<game::Point> force_move;

        for (const game::Point &move : root_moves)
        {
            const SearchBoard::UndoInfo undo = board.apply_move(move.x, move.y, self);
            const TacticalOutcome outcome = classify_tactical_outcome(board, self);
            board.undo_move(undo);

            switch (outcome)
            {
            case TacticalOutcome::WIN:
                return {move, InstantMoveKind::WIN};

            case TacticalOutcome::DRAW:
                if (!draw_move)
                {
                    draw_move = move;
                }
                break;

            case TacticalOutcome::FORCE_LAST_MOVE:
                if (!force_move)
                {
                    force_move = move;
                }
                break;

            default:
                break;
            }
        }

        if (draw_move)
        {
            return {*draw_move, InstantMoveKind::DRAW_SAVE};
        }

        if (force_move)
        {
            return {*force_move, InstantMoveKind::FORCE_LAST_MOVE};
        }

        if (!has_decisive_reply(board, opp))
        {
            return {};
        }

        for (const game::Point &move : root_moves)
        {
            const SearchBoard::UndoInfo undo = board.apply_move(move.x, move.y, self);
            const bool threat_remains = has_decisive_reply(board, opp);
            board.undo_move(undo);

            if (threat_remains)
            {
                continue;
            }

            return {move, InstantMoveKind::BLOCK};
        }

        return {};
    }
} // namespace ttt::my_player
