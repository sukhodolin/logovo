
#include <cstddef>
#include <generator>
#include <string>

// #define TAIL_ALGORITHM_ENABLE_TRACE

#if defined(TAIL_ALGORITHM_ENABLE_TRACE)
#include <spdlog/spdlog.h>
#define TAIL_TRACE(...) spdlog::trace(__VA_ARGS__)
#else
#define TAIL_TRACE(...) \
  do {                  \
  } while (0)
#endif

struct TailParameters {
  size_t BLOCK_SIZE = 4096;
};

// The heart of the project - a function to read a given amount of last lines
// from a given file in line-reversed order.
// Yielded string views remain valid while the generator object is alive and
// until the next yield
template <typename IStream, TailParameters Parameters = TailParameters()>
std::generator<std::string_view> tail(IStream& input, size_t n) {
  std::vector<char> block(Parameters.BLOCK_SIZE);
  input.seekg(0, std::ios_base::end);

  size_t block_start_file_offset;
  size_t block_size;
  std::vector<char>::iterator line_start;
  std::vector<char>::iterator line_end;
  auto read_next_block = [&]() -> bool {
    auto current_pos = input.tellg();
    if (current_pos < 0) {
      TAIL_TRACE("stream in a failed state");
      return false;
    }
    block_size =
        std::min(static_cast<size_t>(current_pos), Parameters.BLOCK_SIZE);
    if (block_size == 0) {
      // The file is empty, nothing to do here
      return false;
    }
    input.seekg(-block_size, std::ios_base::cur);
    block_start_file_offset =
        static_cast<size_t>(static_cast<size_t>(current_pos) - block_size);
    TAIL_TRACE("block_start_file_offset: {}", block_start_file_offset);
    input.read(&block.front(), block_size);
    TAIL_TRACE("read {} bytes starting at offset {}", block_size,
        block_start_file_offset);

    line_end = block.begin() + block_size;
    // We always grab at least one character for the line, hence -2 (-1 to go to
    // the last and another -1 to go to the one before it)
    // But it might be that the block is just the single byte. This indicates
    // we're in the beginning of the file that starts with an empty line.
    line_start = block.begin() + block_size - 1;

    TAIL_TRACE("starting the block: {}, line_start={}, line_end={}",
        fmt::join(block, ","), line_start - block.begin(),
        line_end - block.begin());
    return true;
  };

  if (!read_next_block()) {
    co_return;
  }

  for (;;) {
    while (line_start != block.begin() &&
           // We need to grab at least one symbol
           (*line_start != '\n' || line_start + 1 == line_end)) {
      line_start--;
    }

    TAIL_TRACE("done moving backwards (line_start={}, line_end={})",
        line_start - block.begin(), line_end - block.begin());

    // previous line\nnext line[maybe \n]<remainder>
    //  line start [ ^                   ^ line end )
    while (*line_start == '\n' && line_start + 1 != line_end) {
      auto value = std::string_view(line_start + 1, line_end);
      TAIL_TRACE("yielding {} (line_start={}, line_end={})", value,
          line_start - block.begin() + 1, line_end - block.begin());

      co_yield value;
      if (--n == 0) {
        co_return;
      }

      // Now we want to end up like this (e.g. have a line with a single \n):
      //  previous line\nnext line[maybe \n]
      // line start [ ^  ^ line end )
      line_end = line_start + 1;
      if (line_start != block.begin()) {
        line_start--;
      }
    }
    if (line_start == block.begin()) {
      if (block_start_file_offset == 0 && line_start == block.begin()) {
        // We're dealing with the very first block in the file, yield it as is
        auto value = std::string_view(line_start, line_end);
        TAIL_TRACE("yielding first block {} (line_start={}, line_end={})",
            value, line_start - block.begin(), line_end - block.begin());
        co_yield value;
        // And we've reached the start of the file, so nothing to continue
        co_return;
      }

      // We've reached the beginning of the line and need to fetch another block
      // Our file pointer stands right after the end of the current block and we
      // want to move it to the last yielded line \n
      TAIL_TRACE(
          "block_size: {}, line_end: {}", block_size, line_end - block.begin());
      std::ptrdiff_t rewind = block_size - (line_end - block.begin());
      if (rewind == 0) {
        throw std::runtime_error("Line is longer than the buffer size");
      }
      TAIL_TRACE("rewind: {}", -rewind);
      input.seekg(-rewind, std::ios_base::cur);
      if (!read_next_block()) {
        co_return;
      }
    }
  }
}
