#pragma once

#include <array>
#include <cstdint>
#include <random>

#include "search_board.hpp"

namespace ttt::my_player
{
    class Zobrist
    {
    public:
        static constexpr int kCellKinds = 4;
        static constexpr int kSignKinds = 4;
        static constexpr int kStatusKinds = 4;

        static const Zobrist &instance();

        std::uint64_t cell_key(int index, SearchBoard::Cell cell) const noexcept;
        std::uint64_t current_player_key(game::Sign sign) const noexcept;
        std::uint64_t status_key(game::Status status) const noexcept;
        std::uint64_t winner_key(game::Sign sign) const noexcept;

    private:
        Zobrist();

        static int cell_to_index(SearchBoard::Cell cell) noexcept;
        static int sign_to_index(game::Sign sign) noexcept;
        static int status_to_index(game::Status status) noexcept;

    private:
        std::array<std::array<std::uint64_t, kCellKinds>,
                   SearchBoard::kPaddedCellCount>
            m_cell_keys{};

        std::array<std::uint64_t, kSignKinds> m_current_player_keys{};
        std::array<std::uint64_t, kStatusKinds> m_status_keys{};
        std::array<std::uint64_t, kSignKinds> m_winner_keys{};
    };
}
