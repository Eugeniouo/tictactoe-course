#pragma once

#include "core/game.hpp"
#include "search_board.hpp"

namespace ttt::my_player
{
    /**
     * @brief Тип немедленного тактического результата, найденного до основного поиска.
     */
    enum class InstantMoveKind
    {
        NONE,
        WIN,
        DRAW_SAVE,
        FORCE_LAST_MOVE,
        BLOCK,
    };

    /**
     * @brief Результат быстрого тактического анализа позиции.
     *
     * @details
     * Структура хранит:
     * - координаты найденного хода;
     * - причину, по которой этот ход был выбран.
     *
     * Если подходящий ход не найден, поле `move` остается равным `(-1, -1)`,
     * а `kind` устанавливается в `InstantMoveKind::NONE`.
     */
    struct InstantMoveResult
    {
        game::Point move{-1, -1};
        InstantMoveKind kind = InstantMoveKind::NONE;
    };

    /**
     * @brief Ищет немедленный тактический ход до запуска основного поиска.
     *
     * @param board Текущая поисковая позиция.
     * @return InstantMoveResult Найденный ход и причина его выбора.
     *
     * @details
     * Функция работает в два этапа:
     * - сначала ищет собственный решающий ход текущего игрока;
     * - затем, если такого хода нет, проверяет немедленные ответы соперника
     *   и пытается найти ход, который их полностью устраняет.
     *
     * Под "решающим" здесь понимается ход, который после применения:
     * - немедленно завершает партию победой;
     * - немедленно завершает партию вничью;
     * - переводит позицию в состояние `LAST_MOVE`.
     *
     * @note
     * Проверка опирается на metadata позиции (`game_status`, `winner`),
     * а не только на факт наличия five на сырой доске.
     */
    InstantMoveResult find_instant_move(SearchBoard &board);
} // namespace ttt::my_player
