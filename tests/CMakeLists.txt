find_package(GTest REQUIRED)

set(LOGOVO_TESTS_SOURCES
  test_tail.cc
  main.cc
)

add_executable(logovo_tests ${LOGOVO_TESTS_SOURCES})

target_link_libraries(logovo_tests PRIVATE liblogovo GTest::GTest)
