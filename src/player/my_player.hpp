#pragma once

#include "core/game.hpp"
#include "move_generator.hpp"
#include "negamax_searcher.hpp"
#include "search_state.hpp"

namespace ttt::my_player
{

  using game::Event;
  using game::IPlayer;
  using game::Point;
  using game::Sign;
  using game::State;

  class MyPlayer : public IPlayer
  {
    Sign m_sign = Sign::NONE;
    const char *m_name;

    SearchState m_state;
    MoveGenerator m_move_generator;
    NegamaxSearcher m_negamax_searcher;

    bool m_initialized = false;
    int m_synced_move_number = -1;

  public:
    MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}

    void set_sign(Sign sign) override;
    Point make_move(const State &game) override;
    const char *get_name() const override;

  private:
    void reload_from_state(const State &state, Sign my_sign);
    void sync_from_state(const State &state, Sign my_sign);

    Point choose_best_move();
    void apply_own_move_to_cache(const Point &move, Sign sign);
  };

};
