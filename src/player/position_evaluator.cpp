#include "position_evaluator.hpp"
#include <string>
#include <string_view>

namespace ttt::my_player
{
    namespace
    {
        using Cell = SearchBoard::Cell;

        /**
         * @brief Проверяет, является ли знак игровым камнем одного из игроков.
         *
         * @param sign Проверяемый знак
         * @return true Если знак равен X или O.
         * @return false Если знак не представляет реального игрока.
         */
        bool is_player_sign(game::Sign sign)
        {
            return sign == game::Sign::X || sign == game::Sign::O;
        }

        /**
         * @brief Считает количество вхождений подстроки, включая перекрывающиеся
         *
         * @param haystack Строка, в которой ведётся поиск
         * @param needle Искомый шаблон
         * @return int Количество найденных вхождений
         *
         * @details
         * Ффункция применяется для поиска дырявых паттернов вида "X.XXX",
         * ".XX.X." и похожих шаблонов внутри нормализованной линии
         */
        int count_occurrences(std::string_view haystack, std::string_view needle)
        {
            int count = 0;
            for (std::size_t pos = 0;
                 (pos = haystack.find(needle, pos)) != std::string_view::npos;
                 ++pos)
            {
                ++count;
            }
            return count;
        }

        /**
         * @brief Строит нормализованное строковое представление одной линии доски.
         *
         * @param board Текущее состояние внутренней доски.
         * @param sx Начальная координата x.
         * @param sy Начальная координата y.
         * @param dx Приращение x для перехода к следующей клетке линии.
         * @param dy Приращение y для перехода к следующей клетке линии.
         * @param sign Сторона, относительно которой строится линия.
         * @param out Строка-буфер для результата.
         *
         * @details
         * Линия переводится в алфавит:
         * - "X" — камень рассматриваемой стороны
         * - "." — пустая клетка
         * - "#" — блокирующая клетка: соперник, стена или край линии.
         *
         * С обеих сторон добавляются символы "#", чтобы упростить
         * определение открытых и закрытых паттернов.
         */
        void build_line(
            const SearchBoard &board,
            int sx, int sy, int dx, int dy,
            game::Sign sign,
            std::string &out)

        {
            const Cell own = SearchBoard::cell_from_game_sign(sign);
            out.clear();
            out.push_back('#');
            for (int x = sx, y = sy;
                 SearchBoard::is_within_board(x, y);
                 x += dx, y += dy)
            {
                const Cell c = board.get_cell(x, y);
                if (c == own)
                    out.push_back('X');
                else if (c == Cell::EMPTY)
                    out.push_back('.');
                else
                    out.push_back('#');
            }
            out.push_back('#');
        }

        /**
         * @brief Анализирует непрерывные серии камней в одной линии.
         *
         * @param line Нормализованная линия в алфавите "X", ".", "#".
         * @param counts Структура, в которую накапливаются найденные шаблоны.
         *
         * @details
         * Функция проходит по максимальным подряд идущим сериям `X...X` и,
         * в зависимости от длины серии и числа открытых концов, классифицирует их как:
         * - five
         * - open_four
         * - simple_four
         * - open_three
         * - simple_three
         * - open_two
         * - closed_two
         *
         * @note
         * Данный проход отвечает только за непрерывные шаблоны, без внутренних разрывов.
         */
        void analyze_run_patterns(std::string_view line, PatternCounts &counts)
        {
            for (std::size_t i = 1; i + 1 < line.size();)
            {
                if (line[i] != 'X')
                {
                    ++i;
                    continue;
                }

                std::size_t j = i;
                while (j < line.size() && line[j] == 'X')
                    ++j;

                const int len = static_cast<int>(j - i);
                const bool left_open = (line[i - 1] == '.');
                const bool right_open = (line[j] == '.');
                const int open_ends = static_cast<int>(left_open) + static_cast<int>(right_open);

                if (len >= 5)
                {
                    ++counts.five;
                }
                else if (len == 4)
                {
                    if (open_ends == 2)
                        ++counts.open_four;
                    else if (open_ends == 1)
                        ++counts.simple_four;
                }
                else if (len == 3)
                {
                    if (open_ends == 2)
                        ++counts.open_three;
                    else if (open_ends == 1)
                        ++counts.simple_three;
                }
                else if (len == 2)
                {
                    if (open_ends == 2)
                        ++counts.open_two;
                    else if (open_ends == 1)
                        ++counts.closed_two;
                }

                i = j;
            }
        }

        /**
         * @brief Анализирует шаблоны с одним внутренним разрывом.
         *
         * @param line Нормализованная линия в алфавите
         * @param counts Структура, в которую накапливаются найденные шаблоны
         *
         * @details
         * Функция распознаёт более сложные формы угроз:
         * - broken / gapped four, XX.XX, X.XXX, XXX.X;
         * - broken three, .XX.X., .X.XX.;
         * - broken two, .X.X..
         */
        void analyze_gap_patterns(std::string_view line, PatternCounts &counts)
        {
            // Winning broken fours (one gap)
            counts.simple_four += count_occurrences(line, "XX.XX");
            counts.simple_four += count_occurrences(line, "X.XXX");
            counts.simple_four += count_occurrences(line, "XXX.X");

            // Open broken threes
            counts.broken_three += count_occurrences(line, ".XX.X.");
            counts.broken_three += count_occurrences(line, ".X.XX.");

            // One-sided broken threes
            counts.simple_three += count_occurrences(line, "#XX.X.");
            counts.simple_three += count_occurrences(line, ".XX.X#");
            counts.simple_three += count_occurrences(line, "#X.XX.");
            counts.simple_three += count_occurrences(line, ".X.XX#");

            // Open broken twos
            counts.broken_two += count_occurrences(line, ".X.X.");

            // One-sided broken twos
            counts.closed_two += count_occurrences(line, "#X.X.");
            counts.closed_two += count_occurrences(line, ".X.X#");
        }

        /**
         * @brief Собирает полную статистику паттернов для одной стороны по всей доске
         *
         * @param board Текущее состояние внутренней доски
         * @param sign Сторона, для которой собираются шаблон
         * @return PatternCounts Итоговые счётчики всех найденных паттернов
         *
         * @details
         * Метод проходит по всем уникальным линиям в четырёх направлениях:
         * - горизонталь
         * - вертикаль
         * - диагонали (2)
         *
         * Для каждой линии строится нормализованная строка, после чего к ней
         * применяются:
         * - анализ непрерывных серий
         * - анализ gap-паттернов
         */
        PatternCounts collect_pattern_counts(const SearchBoard &board, game::Sign sign)
        {
            PatternCounts counts;
            if (!is_player_sign(sign))
                return counts;

            std::string line;
            line.reserve(SearchBoard::kBoardWidth + 2);

            const auto process = [&](int sx, int sy, int dx, int dy)
            {
                build_line(board, sx, sy, dx, dy, sign, line);
                if (line.size() < 4)
                    return;
                analyze_run_patterns(line, counts);
                analyze_gap_patterns(line, counts);
            };

            for (int y = 0; y < SearchBoard::kBoardHeight; ++y)
                process(0, y, 1, 0);

            for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
                process(x, 0, 0, 1);

            for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
                process(x, 0, 1, 1);
            for (int y = 1; y < SearchBoard::kBoardHeight; ++y)
                process(0, y, 1, 1);

            for (int x = 0; x < SearchBoard::kBoardWidth; ++x)
                process(x, SearchBoard::kBoardHeight - 1, 1, -1);
            for (int y = 0; y < SearchBoard::kBoardHeight - 1; ++y)
                process(0, y, 1, -1);

            return counts;
        }

        Score score_pattern_counts(const PatternCounts &c)
        {
            Score s = 0;

            s += static_cast<Score>(c.five) * kFive;
            s += static_cast<Score>(c.open_four) * kOpenFour;
            s += static_cast<Score>(c.simple_four) * kSimpleFour;
            s += static_cast<Score>(c.open_three) * kOpenThree;
            s += static_cast<Score>(c.broken_three) * kBrokenThree;
            s += static_cast<Score>(c.simple_three) * kSimpleThree;
            s += static_cast<Score>(c.open_two) * kOpenTwo;
            s += static_cast<Score>(c.broken_two) * kBrokenTwo;
            s += static_cast<Score>(c.closed_two) * kClosedTwo;

            const int total_fours = c.open_four + c.simple_four;
            if (total_fours >= 2)
                s += static_cast<Score>(total_fours - 1) * kDoubleFourBonus;

            if (c.open_three >= 2)
                s += static_cast<Score>(c.open_three - 1) * kDoubleOpenThreeBonus;

            if (c.open_two >= 2)
                s += static_cast<Score>(c.open_two - 1) * kDoubleOpenTwoBonus;

            return s;
        }

    } // namespace

    /**
     * @brief Вычисляет подробную эвристику позиции для заданной стороны.
     *
     * @param board Текущее внутреннее представление позиции.
     * @param perspective Сторона, относительно которой считается оценка.
     * @return EvalBreakdown Полная расшифровка оценки позиции.
     *
     * @details
     * Метод:
     * - проверяет терминальную позицию
     * - сканирует все линии по четырём направлениям
     * - считает паттерны для себя и для соперника
     * - переводит их в численные оценки
     * - возвращает итоговое значение self_score - opponent_score
     *
     * Положительный итог означает, что позиция выгодна для себя.
     * Отрицательный итог означает преимущество соперника.
     */
    EvalBreakdown evaluate_breakdown(const SearchBoard &board, game::Sign perspective)
    {
        EvalBreakdown result;
        if (!is_player_sign(perspective))
            return result;

        if (board.game_status() == game::Status::ENDED)
        {
            const game::Sign opp = SearchBoard::opposite_player_sign(perspective);
            if (board.winner() == perspective)
                result.final_score = kTerminalWinScore;
            else if (board.winner() == opp)
                result.final_score = -kTerminalWinScore;

            return result;
        }

        const game::Sign opp = SearchBoard::opposite_player_sign(perspective);

        result.self = collect_pattern_counts(board, perspective);
        result.opponent = collect_pattern_counts(board, opp);

        result.self_score = score_pattern_counts(result.self);
        result.opponent_score = score_pattern_counts(result.opponent);
        result.final_score = result.self_score - result.opponent_score;

        return result;
    }

    /**
     * @brief Вычисляет подробную эвристику позиции для board.my_sign()
     *
     * @param board Текущее внутреннее представление позиции
     * @return EvalBreakdown Полная расшифровка оценки позиции
     *
     * @details
     * Перегрузка для случаев, когда evaluator вызывается относительно
     * стороны, уже сохранённой внутри SearchBoard.
     */
    EvalBreakdown evaluate_breakdown(const SearchBoard &board)
    {
        return evaluate_breakdown(board, board.my_sign());
    }

    /**
     * @brief Вычисляет только итоговую численную оценку позиции
     *
     * @param board Текущее внутреннее представление позиции
     * @param perspective Сторона, относительно которой считается оценка
     * @return Score Итоговая эвристическая оценка
     *
     * @note
     * Это компактный интерфейс для поискового алгоритма
     * Обычно именно эта функция будет вызываться из negamax / alpha-beta на листьях поиска
     */
    Score evaluate_position(const SearchBoard &board, game::Sign perspective)
    {
        return evaluate_breakdown(board, perspective).final_score;
    }

    /**
     * @brief Вычисляет итоговую оценку позиции для board.my_sign()
     *
     * @param board Текущее внутреннее представление позиции
     * @return Score Итоговая эвристическая оценка
     */
    Score evaluate_position(const SearchBoard &board)
    {
        return evaluate_breakdown(board).final_score;
    }

} // namespace ttt::my_player