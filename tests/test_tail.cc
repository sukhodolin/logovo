#include <gtest/gtest.h>
#include <liblogovo/tail.h>

TEST(Tail, Empty) {
  std::stringstream input("");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  GTEST_ASSERT_TRUE(last_lines.empty());
}

TEST(Tail, CarriageReturn) {
  std::stringstream input("\n");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  std::vector<std::string> expected{"\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, CarriageReturn2) {
  std::stringstream input("\n\n");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  std::vector<std::string> expected{"\n", "\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, CarriageReturn3) {
  std::stringstream input("\n\n\n");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  std::vector<std::string> expected{"\n", "\n", "\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, OneLine) {
  std::stringstream input("line");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  std::vector<std::string> expected{"line"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, TwoLines) {
  std::stringstream input(R"(
line1
line2)");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  std::vector<std::string> expected{"line2", "line1\n", "\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, SmallBlock) {
  std::stringstream input(R"(
line1
line2)");
  // Minimal block size for the input above is 7 bytes to ensure that we can
  // fit both trailing and leading \ns for the longest string - e.g.
  // \n l  i  n  e  1  \n
  // 1  2  3  4  5  6  7
  auto last_lines = std::ranges::to<std::vector<std::string>>(
      tail<std::stringstream, TailParameters{7}>(input, 5));
  std::vector<std::string> expected{"line2", "line1\n", "\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, SmokeTest) {
  std::stringstream input(R"(
The
quick
brown
fox
jumps
over
the
lazy
dog
)");
  auto last_lines = std::ranges::to<std::vector<std::string>>(tail(input, 5));
  std::vector<std::string> expected = {
      "dog\n",
      "lazy\n",
      "the\n",
      "over\n",
      "jumps\n",
  };

  GTEST_ASSERT_EQ(last_lines, expected);
}
