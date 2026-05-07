#pragma once

#include <cstdint>
#include <vector>

#include "core/game.hpp"
#include "engine_config.hpp"
#include "move_generator.hpp"
#include "search_state.hpp"

namespace ttt::my_player
{

    struct NegamaxResult
    {
        bool found = false;
        game::Point best_move{};
        int nodes = 0;
    };

    enum class TTFlag : std::uint8_t
    {
        EXACT = 0,
        LOWERBOUND,
        UPPERBOUND
    };

    struct TTEntry
    {
        std::uint64_t key = 0;
        Score score = 0;
        int depth = -1;
        TTFlag flag = TTFlag::EXACT;
        std::uint16_t move_index = 0;
    };

    class NegamaxSearcher
    {
    public:
        NegamaxSearcher();

        NegamaxResult find_best_move(
            SearchState &state,
            game::Sign side,
            const MoveList &root_moves) noexcept;

    private:
        Score negamax(
            SearchState &state,
            game::Sign side,
            int depth,
            Score alpha,
            Score beta,
            int ply) noexcept;

        Score quiescence_search(
            SearchState &state,
            game::Sign side,
            Score alpha,
            Score beta,
            int ply,
            int qs_depth) noexcept;

        Score evaluate_leaf(
            const SearchState &state,
            game::Sign side) const noexcept;

        bool terminal_score(
            const SearchState &state,
            game::Sign side,
            int ply,
            Score &score) const noexcept;

        static bool is_playable_side(game::Sign side) noexcept;

        Score adjust_score_for_tt(Score score, int ply) const noexcept;
        Score adjust_score_from_tt(Score score, int ply) const noexcept;

        void store_tt(
            std::uint64_t key,
            int depth,
            int ply,
            Score score,
            TTFlag flag,
            std::uint16_t move_index) noexcept;

        bool probe_tt(
            std::uint64_t key,
            int depth,
            int ply,
            Score alpha,
            Score beta,
            Score &tt_score,
            std::uint16_t &tt_move_index) const noexcept;

        static constexpr int kTTSize = 1 << 20;
        static constexpr int kTTMask = kTTSize - 1;

        std::vector<TTEntry> m_tt;
        MoveGenerator m_move_generator;
        int m_nodes = 0;
        bool m_completed = true;
    };

}
