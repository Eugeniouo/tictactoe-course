/**
 * @file negamax_searcher.cpp
 * @brief Реализация negamax-поиска с PVS отсечением
 *
 * Файл перебирает корневые ходы с итеративным углублением, использует principal
 * variation search, quiescence search для острых позиций и таблицу транспозиций.
 * Ходы применяются к SearchState и затем откатываются.
 *
 */

#include "negamax_searcher.hpp"

#include <algorithm>
#include <utility>

namespace ttt::my_player
{

    NegamaxSearcher::NegamaxSearcher() : m_tt(kTTSize)
    {
    }

    NegamaxResult NegamaxSearcher::find_best_move(
        SearchState &state,
        game::Sign side,
        const MoveList &root_moves) noexcept
    {
        m_nodes = 0;
        m_completed = true;

        NegamaxResult best_result;

        if (!is_playable_side(side) || root_moves.empty())
        {
            return best_result;
        }

        best_result.found = true;
        best_result.best_move = root_moves[0].point;

        const game::Sign opponent = SearchBoard::opposite_player_sign(side);

        MoveList current_root_moves = root_moves;

        for (int depth = 1; depth <= EngineConfig::kSearchMaxDepth; ++depth)
        {
            if (m_nodes >= EngineConfig::kSearchNodeLimit)
            {
                m_completed = false;
                break;
            }

            Score alpha = -EngineConfig::kSearchInfinity;
            const Score beta = EngineConfig::kSearchInfinity;

            Score best_score = -EngineConfig::kSearchInfinity;
            std::uint16_t best_move_index = 0;
            game::Point best_move_point{};
            int best_move_list_index = -1;

            bool depth_completed = true;
            bool is_first_move = true;

            for (int i = 0; i < current_root_moves.count; ++i)
            {
                if (m_nodes >= EngineConfig::kSearchNodeLimit)
                {
                    depth_completed = false;
                    m_completed = false;
                    break;
                }

                const ScoredMove &move = current_root_moves[i];

                const SearchState::Undo undo =
                    state.apply_move(move.point.x, move.point.y, side);

                Score score;
                if (is_first_move)
                {
                    score = -negamax(state, opponent, depth - 1, -beta, -alpha, 1);
                    is_first_move = false;
                }
                else
                {
                    score = -negamax(state, opponent, depth - 1, -alpha - 1, -alpha, 1);

                    if (score > alpha && score < beta)
                    {
                        score = -negamax(state, opponent, depth - 1, -beta, -alpha, 1);
                    }
                }

                state.undo_move(undo);

                if (score > best_score)
                {
                    best_score = score;
                    best_move_index = move.index;
                    best_move_point = move.point;
                    best_move_list_index = i;
                }

                if (score > alpha)
                {
                    alpha = score;
                }
            }

            if (depth_completed && best_move_list_index != -1)
            {
                best_result.found = true;
                best_result.best_move = best_move_point;

                store_tt(state.board().hash(), depth, 0, best_score, TTFlag::EXACT, best_move_index);

                if (best_move_list_index > 0)
                {
                    ScoredMove best_scored_move = current_root_moves[best_move_list_index];
                    for (int i = best_move_list_index; i > 0; --i)
                    {
                        current_root_moves.moves[i] = current_root_moves.moves[i - 1];
                    }
                    current_root_moves.moves[0] = best_scored_move;
                }
            }
        }

        best_result.nodes = m_nodes;
        return best_result;
    }

    Score NegamaxSearcher::negamax(
        SearchState &state,
        game::Sign side,
        int depth,
        Score alpha,
        Score beta,
        int ply) noexcept
    {
        ++m_nodes;

        const std::uint64_t hash_key = state.board().hash();

        Score tt_score = 0;
        std::uint16_t tt_move_index = 0;
        if (probe_tt(hash_key, depth, ply, alpha, beta, tt_score, tt_move_index))
        {
            return tt_score;
        }

        Score terminal = 0;
        if (terminal_score(state, side, ply, terminal))
        {
            return terminal;
        }

        if (depth <= 0 || m_nodes >= EngineConfig::kSearchNodeLimit)
        {
            if (m_nodes >= EngineConfig::kSearchNodeLimit)
            {
                m_completed = false;
            }

            const Score leaf_score = quiescence_search(state, side, alpha, beta, ply, 0);
            store_tt(hash_key, depth, ply, leaf_score, TTFlag::EXACT, 0);
            return leaf_score;
        }

        MoveList moves =
            m_move_generator.generate(
                state,
                side,
                EngineConfig::kSearchInnerQuietMoves);

        if (moves.empty())
        {
            const Score leaf_score = quiescence_search(state, side, alpha, beta, ply, 0);
            store_tt(hash_key, depth, ply, leaf_score, TTFlag::EXACT, 0);
            return leaf_score;
        }

        if (tt_move_index != 0)
        {
            for (int i = 0; i < moves.count; ++i)
            {
                if (moves[i].index == tt_move_index)
                {
                    if (i > 0)
                    {
                        std::swap(moves.moves[0], moves.moves[i]);
                    }
                    break;
                }
            }
        }

        const game::Sign opponent = SearchBoard::opposite_player_sign(side);

        Score best_score = -EngineConfig::kSearchInfinity;
        std::uint16_t best_move_index = 0;
        const Score original_alpha = alpha;
        bool is_first_move = true;

        for (int i = 0; i < moves.count; ++i)
        {
            if (m_nodes >= EngineConfig::kSearchNodeLimit)
            {
                m_completed = false;
                break;
            }

            const ScoredMove &move = moves[i];

            const SearchState::Undo undo =
                state.apply_move(move.point.x, move.point.y, side);

            Score score;
            if (is_first_move)
            {
                score = -negamax(state, opponent, depth - 1, -beta, -alpha, ply + 1);
                is_first_move = false;
            }
            else
            {
                score = -negamax(state, opponent, depth - 1, -alpha - 1, -alpha, ply + 1);

                if (score > alpha && score < beta)
                {
                    score = -negamax(state, opponent, depth - 1, -beta, -alpha, ply + 1);
                }
            }

            state.undo_move(undo);

            if (score > best_score)
            {
                best_score = score;
                best_move_index = move.index;
            }

            if (score > alpha)
            {
                alpha = score;
            }

            if (alpha >= beta)
            {
                break;
            }
        }

        if (m_completed || best_score > -EngineConfig::kSearchInfinity)
        {
            TTFlag flag = TTFlag::EXACT;
            if (best_score <= original_alpha)
            {
                flag = TTFlag::UPPERBOUND;
            }
            else if (best_score >= beta)
            {
                flag = TTFlag::LOWERBOUND;
            }

            store_tt(hash_key, depth, ply, best_score, flag, best_move_index);
        }

        return best_score;
    }

    Score NegamaxSearcher::quiescence_search(
        SearchState &state,
        game::Sign side,
        Score alpha,
        Score beta,
        int ply,
        int qs_depth) noexcept
    {
        ++m_nodes;

        Score terminal = 0;
        if (terminal_score(state, side, ply, terminal))
        {
            return terminal;
        }

        if (m_nodes >= EngineConfig::kSearchNodeLimit)
        {
            m_completed = false;
            return evaluate_leaf(state, side);
        }

        Score stand_pat = evaluate_leaf(state, side);

        if (stand_pat >= beta)
        {
            return stand_pat;
        }

        if (alpha < stand_pat)
        {
            alpha = stand_pat;
        }

        if (qs_depth > EngineConfig::kQuiescenceMaxDepth)
        {
            return stand_pat;
        }

        MoveList moves =
            m_move_generator.generate(
                state,
                side,
                0);

        const game::Sign opponent = SearchBoard::opposite_player_sign(side);

        Score best_score = stand_pat;

        for (int i = 0; i < moves.count; ++i)
        {
            const ScoredMove &move = moves[i];

            if (move.tier > MoveTier::BLOCK_OPEN_FOUR)
            {
                break;
            }

            const SearchState::Undo undo =
                state.apply_move(move.point.x, move.point.y, side);

            const Score score =
                -quiescence_search(
                    state,
                    opponent,
                    -beta,
                    -alpha,
                    ply + 1,
                    qs_depth + 1);

            state.undo_move(undo);

            if (score > best_score)
            {
                best_score = score;
            }

            if (score > alpha)
            {
                alpha = score;
            }

            if (alpha >= beta)
            {
                break;
            }
        }

        return best_score;
    }

    Score NegamaxSearcher::evaluate_leaf(
        const SearchState &state,
        game::Sign side) const noexcept
    {
        return state.evaluator().evaluate(side);
    }

    bool NegamaxSearcher::terminal_score(
        const SearchState &state,
        game::Sign side,
        int ply,
        Score &score) const noexcept
    {
        if (state.game_status() != game::Status::ENDED)
        {
            return false;
        }

        const game::Sign winner = state.winner();

        if (winner == game::Sign::NONE)
        {
            score = 0;
            return true;
        }

        if (winner == side)
        {
            score = EngineConfig::kSearchMateScore - ply;
        }
        else
        {
            score = -EngineConfig::kSearchMateScore + ply;
        }

        return true;
    }

    bool NegamaxSearcher::is_playable_side(game::Sign side) noexcept
    {
        return side == game::Sign::X || side == game::Sign::O;
    }

    Score NegamaxSearcher::adjust_score_for_tt(Score score, int ply) const noexcept
    {
        if (score >= EngineConfig::kSearchMateScore - 1000)
        {
            return score + ply;
        }
        if (score <= -EngineConfig::kSearchMateScore + 1000)
        {
            return score - ply;
        }
        return score;
    }

    Score NegamaxSearcher::adjust_score_from_tt(Score score, int ply) const noexcept
    {
        if (score >= EngineConfig::kSearchMateScore - 1000)
        {
            return score - ply;
        }
        if (score <= -EngineConfig::kSearchMateScore + 1000)
        {
            return score + ply;
        }
        return score;
    }

    void NegamaxSearcher::store_tt(
        std::uint64_t key,
        int depth,
        int ply,
        Score score,
        TTFlag flag,
        std::uint16_t move_index) noexcept
    {
        TTEntry &entry = m_tt[key & kTTMask];

        if (entry.key == key && entry.depth > depth && flag != TTFlag::EXACT)
        {
            return;
        }

        entry.key = key;
        entry.depth = depth;
        entry.score = adjust_score_for_tt(score, ply);
        entry.flag = flag;
        entry.move_index = move_index;
    }

    bool NegamaxSearcher::probe_tt(
        std::uint64_t key,
        int depth,
        int ply,
        Score alpha,
        Score beta,
        Score &tt_score,
        std::uint16_t &tt_move_index) const noexcept
    {
        const TTEntry &entry = m_tt[key & kTTMask];

        if (entry.key != key)
        {
            return false;
        }

        tt_move_index = entry.move_index;

        if (entry.depth < depth)
        {
            return false;
        }

        const Score score = adjust_score_from_tt(entry.score, ply);

        if (entry.flag == TTFlag::EXACT)
        {
            tt_score = score;
            return true;
        }
        if (entry.flag == TTFlag::LOWERBOUND && score >= beta)
        {
            tt_score = score;
            return true;
        }
        if (entry.flag == TTFlag::UPPERBOUND && score <= alpha)
        {
            tt_score = score;
            return true;
        }

        return false;
    }

}
