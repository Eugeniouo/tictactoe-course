#pragma once
#include <cstdint>
#include "core/game.hpp"
#include "search_board.hpp"

namespace ttt::my_player
{
    using Score = std::int64_t;

    inline constexpr Score kTerminalWinScore = 4'000'000'000LL;
    inline constexpr Score kFive = 1'000'000'000LL;
    inline constexpr Score kOpenFour = 10'000'000LL;
    inline constexpr Score kDoubleFourBonus = 5'000'000LL;
    inline constexpr Score kSimpleFour = 1'000'000LL;
    inline constexpr Score kDoubleOpenThreeBonus = 300'000LL;
    inline constexpr Score kOpenThree = 100'000LL;
    inline constexpr Score kBrokenThree = 40'000LL;
    inline constexpr Score kSimpleThree = 10'000LL;
    inline constexpr Score kDoubleOpenTwoBonus = 2'000LL;
    inline constexpr Score kOpenTwo = 1'000LL;
    inline constexpr Score kBrokenTwo = 300LL;
    inline constexpr Score kClosedTwo = 100LL;

    /**
     * @brief Количество обнаруженных шаблонов каждого типа для одной стороны.
     *
     * @note
     * Структура хранит только сырые счётчики паттернов, без применения весов.
     * Используется как промежуточное представление между:
     * - этапом сканирования линий;
     * - этапом вычисления итогового численного score.
     */
    struct PatternCounts
    {
        int five = 0;
        int open_four = 0;
        int simple_four = 0;
        int open_three = 0;
        int broken_three = 0;
        int simple_three = 0;
        int open_two = 0;
        int broken_two = 0;
        int closed_two = 0;
    };

    /**
     * @brief Подробный результат эвристической оценки позиции.
     *
     * @details
     * Структура хранит:
     * - найденные шаблоны для себя
     * - найденные шаблоны для соперника
     * - взвешенные суммы для обеих сторон;
     * - итоговую разницу `self_score - opponent_score`.
     */
    struct EvalBreakdown
    {
        PatternCounts self;
        PatternCounts opponent;
        Score self_score = 0;
        Score opponent_score = 0;
        Score final_score = 0;
    };

    EvalBreakdown evaluate_breakdown(const SearchBoard &board, game::Sign perspective);
    EvalBreakdown evaluate_breakdown(const SearchBoard &board);
    Score evaluate_position(const SearchBoard &board, game::Sign perspective);
    Score evaluate_position(const SearchBoard &board);
}