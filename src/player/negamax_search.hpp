#pragma once

#include <cstdint>
#include "core/game.hpp"
#include "search_board.hpp"
#include "position_evaluator.hpp"

namespace ttt::my_player
{
    struct SearchResult
    {
        game::Point best_move{-1, -1};
        Score best_score = 0;
        std::uint64_t visited_nodes = 0;
    };

    /**
     * @brief Находит лучший ход фиксированным поиском negamax с alpha-beta отсечением.
     *
     * @param board Поисковая позиция, от которой запускается поиск.
     * @param depth Глубина поиска.
     * @return SearchResult Лучший найденный ход, его оценка и число просмотренных узлов.
     *
     * @details
     * Функция запускает корневой перебор всех кандидатных ходов, для каждого
     * вызывает рекурсивный negamax и выбирает ход с максимальной оценкой.
     *
     * @note
     * MyPlayer создает SearchBoard, вызывает эту функцию и получает ответ.
     */
    SearchResult find_best_move(SearchBoard &board, int depth);
}
