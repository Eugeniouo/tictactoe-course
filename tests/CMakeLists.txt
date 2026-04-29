cmake_minimum_required(VERSION 3.20.0)

include(FetchContent)

if(NOT TARGET GTest::gtest_main)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  )

  FetchContent_MakeAvailable(googletest)
endif()

# Always built targets (no dependency on BUILD_TTTCORE)
add_executable(test_my_player test_my_player.cpp)
target_link_libraries(test_my_player tttplayer)
add_test(NAME smoke_test_player COMMAND ./test_my_player)

add_executable(test_stats test_stats.cpp)
target_link_libraries(test_stats tttplayer)
add_test(NAME test_player_stats COMMAND ./test_stats)

add_executable(test_search_board test_search_board.cpp)
target_link_libraries(test_search_board tttplayer GTest::gtest_main)
add_test(NAME test_search_board COMMAND test_search_board)

add_executable(test_move_generator test_move_generator.cpp)
target_link_libraries(test_move_generator tttplayer GTest::gtest_main)
add_test(NAME test_move_generator COMMAND test_move_generator)

add_executable(test_position_evaluator test_position_evaluator.cpp)
target_link_libraries(test_position_evaluator tttplayer GTest::gtest_main)
add_test(NAME test_position_evaluator COMMAND test_position_evaluator)

add_executable(test_negamax_search test_negamax_search.cpp)
target_link_libraries(test_negamax_search tttplayer GTest::gtest_main)
add_test(NAME test_negamax_search COMMAND test_negamax_search)

# Targets that require full or prebuilt tttcore
if((BUILD_TTTCORE STREQUAL "FULL") OR (BUILD_TTTCORE STREQUAL "PREBUILT"))
  # Baseline tests
  add_executable(test_with_baseline test_baseline.cpp)
  target_link_libraries(test_with_baseline tttplayer)
  add_test(NAME test_againts_baseline COMMAND ./test_with_baseline)

  # New stat test
  add_executable(test_stats_vs_baseline test_stats_vs_baseline.cpp)
  target_link_libraries(test_stats_vs_baseline tttplayer)
  add_test(NAME test_stats_vs_baseline COMMAND ./test_stats_vs_baseline)
  
  # Human player test
  add_executable(test_my_player_vs_human test_my_player_vs_human.cpp human_player.cpp)
  target_link_libraries(test_my_player_vs_human tttplayer)
endif()
