/**
 * @file zobrist.cpp
 * @brief Реализация детерминированной генерации zobristключей
 *
 * Zobrist хранит детерминированные случайные ключи для клеток, текущего
 * игрока, статуса партии и победителя. SearchBoard использует эти ключи,
 * чтобы быстро обновлять хеш при применении и откате ходов.
 */

#include "zobrist.hpp"

namespace ttt::my_player
{

    const Zobrist &Zobrist::instance()
    {
        static const Zobrist zobrist;
        return zobrist;
    }

    Zobrist::Zobrist()
    {
        std::mt19937_64 rng(0xC0FFEE123456789ULL);

        for (auto &keys_by_cell : m_cell_keys)
        {
            for (std::uint64_t &key : keys_by_cell)
            {
                key = rng();
            }
        }

        for (std::uint64_t &key : m_current_player_keys)
        {
            key = rng();
        }

        for (std::uint64_t &key : m_status_keys)
        {
            key = rng();
        }

        for (std::uint64_t &key : m_winner_keys)
        {
            key = rng();
        }
    }

    std::uint64_t Zobrist::cell_key(int index, SearchBoard::Cell cell) const noexcept
    {
        return m_cell_keys[index][cell_to_index(cell)];
    }

    std::uint64_t Zobrist::current_player_key(game::Sign sign) const noexcept
    {
        return m_current_player_keys[sign_to_index(sign)];
    }

    std::uint64_t Zobrist::status_key(game::Status status) const noexcept
    {
        return m_status_keys[status_to_index(status)];
    }

    std::uint64_t Zobrist::winner_key(game::Sign sign) const noexcept
    {
        return m_winner_keys[sign_to_index(sign)];
    }

    int Zobrist::cell_to_index(SearchBoard::Cell cell) noexcept
    {
        switch (cell)
        {
        case SearchBoard::Cell::EMPTY:
            return 0;
        case SearchBoard::Cell::X:
            return 1;
        case SearchBoard::Cell::O:
            return 2;
        case SearchBoard::Cell::WALL:
            return 3;
        default:
            return 0;
        }
    }

    int Zobrist::sign_to_index(game::Sign sign) noexcept
    {
        switch (sign)
        {
        case game::Sign::NONE:
            return 0;
        case game::Sign::X:
            return 1;
        case game::Sign::O:
            return 2;
        case game::Sign::WALL:
            return 3;
        default:
            return 0;
        }
    }

    int Zobrist::status_to_index(game::Status status) noexcept
    {
        switch (status)
        {
        case game::Status::CREATED:
            return 0;
        case game::Status::ACTIVE:
            return 1;
        case game::Status::LAST_MOVE:
            return 2;
        case game::Status::ENDED:
            return 3;
        default:
            return 0;
        }
    }

}
