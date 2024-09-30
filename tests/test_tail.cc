#include <gtest/gtest.h>
#include <liblogovo/tail.h>

TEST(Tail, Empty) {
  std::stringstream input("");
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
  GTEST_ASSERT_TRUE(last_lines.empty());
}

TEST(Tail, CarriageReturn) {
  std::stringstream input("\n");
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
  std::vector<std::string> expected{"\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, CarriageReturn2) {
  std::stringstream input("\n\n");
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
  std::vector<std::string> expected{"\n", "\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, CarriageReturn3) {
  std::stringstream input("\n\n\n");
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
  std::vector<std::string> expected{"\n", "\n", "\n"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, OneLine) {
  std::stringstream input("line");
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
  std::vector<std::string> expected{"line"};
  GTEST_ASSERT_EQ(last_lines, expected);
}

TEST(Tail, TwoLines) {
  std::stringstream input(R"(
line1
line2)");
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
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

  auto result = tail<std::stringstream, TailParameters{7}>(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
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
  auto result = tail(input, 5);
  std::vector<std::string> last_lines;
  for (auto item : result) {
    last_lines.push_back(std::string(item));
  }
  std::vector<std::string> expected = {
      "dog\n",
      "lazy\n",
      "the\n",
      "over\n",
      "jumps\n",
  };

  GTEST_ASSERT_EQ(last_lines, expected);
}
