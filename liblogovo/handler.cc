#include "handler.h"

#include <spdlog/spdlog.h>

#include <boost/beast/version.hpp>
#include <boost/url/parse.hpp>
#include <fstream>

#include "tail.h"
#include "vendor/generator.h"

namespace beast = boost::beast;
namespace http = beast::http;

// Maximum amount of lines that can be requested. Exceeding this will result in
// bad request
constexpr size_t REQUEST_MAX_N = 1000000;

Handler::Handler(std::filesystem::path root_dir) : root_dir_(root_dir) {}

auto bad_request(
    http::request<http::string_body>& req, beast::string_view why) {
  http::response<http::string_body> res{
      http::status::bad_request, req.version()};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/html");
  res.keep_alive(req.keep_alive());
  res.body() = std::string(why);
  res.prepare_payload();
  return res;
};

auto not_found(http::request<http::string_body>& req) {
  http::response<http::string_body> res{http::status::not_found, req.version()};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/html");
  res.keep_alive(req.keep_alive());
  res.body() = std::string("Not found");
  res.prepare_payload();
  return res;
};

auto internal_server_error(
    http::request<http::string_body>& req, beast::string_view why) {
  http::response<http::string_body> res{
      http::status::internal_server_error, req.version()};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/html");
  res.keep_alive(req.keep_alive());
  res.body() = std::string(why);
  res.prepare_payload();
  return res;
};

boost::beast::http::message_generator Handler::handle_request(
    boost::beast::http::request<boost::beast::http::string_body>&& req) {
  spdlog::info("Request: {} {}",
      std::string(req.method_string().data(), req.method_string().size()),
      std::string(req.target().data(), req.target().size()));

  try {
    auto res = handle_request_(std::move(req));
    return res;
  } catch (const std::exception& e) {
    spdlog::error("Exception in request handler: {}", e.what());
    return internal_server_error(req, "Internal server error");
  }
}

struct LogStream {
  std::ifstream input_stream;
  std::function<bool(std::string_view)> grep;
  std::generator<std::string_view> generator;
};

struct LogBodyWriter {
 public:
  using const_buffers_type = beast::net::const_buffer;

  template <bool isRequest, typename Fields>
  LogBodyWriter(http::header<isRequest, Fields> const& h,
      std::unique_ptr<LogStream>& body)
      : log_stream_(body.get()) {}

  void init(beast::error_code& ec) { ec = {}; }

  boost::optional<std::pair<const_buffers_type, bool>> get(
      beast::error_code& ec) {
    std::optional<std::string_view> next;
    while (!next) {
      if (!maybe_current_) {
        maybe_current_ = log_stream_->generator.begin();
      } else {
        (*maybe_current_)++;
      }
      auto& current = *maybe_current_;

      if (current == log_stream_->generator.end()) {
        ec = {};
        return boost::none;
      }

      ec = {};
      if (log_stream_->grep(*current)) {
        next = *current;
      }
    }

    // Sending lines one at a time might not be the most efficient thing, so an
    // obvious way to improve performance if needed is to add buffering here at
    // the writer.
    auto result_buffer = beast::net::const_buffer(next->data(), next->size());

    return std::make_pair(result_buffer, true);
  }

 private:
  std::optional<std::generator<std::string_view>::iterator> maybe_current_;
  LogStream* log_stream_;
};

struct LogBody {
  using value_type = std::unique_ptr<LogStream>;
  using writer = LogBodyWriter;
};

// Strongly-typed data of the GET request supported by logovo
struct LogRequest {
  std::filesystem::path file_path;
  std::optional<size_t> maybe_n;
  std::optional<std::string> maybe_grep;
};

// Parses a GET request, validates it and returns LogRequest
std::optional<LogRequest> parse_log_request(beast::string_view target) {
  auto origin_form = boost::urls::parse_origin_form(target);
  if (origin_form.has_error()) {
    spdlog::warn("Failed to parse URL: {}", origin_form.error().message());
    return std::nullopt;
  }

  LogRequest result;

  // We're converting path to lexically normal here to avoid any trickery
  // like
  // ./../../<something>
  result.file_path =
      std::filesystem::path(origin_form->path()).lexically_normal();

  auto params_n = origin_form->params().find("n");
  if (params_n != origin_form->params().end()) {
    int n = std::atoi((*params_n).value.c_str());
    if (n < 0) {
      return std::nullopt;
    }
    if (n > REQUEST_MAX_N) {
      return std::nullopt;
    }

    result.maybe_n = static_cast<size_t>(n);
  }

  auto params_grep = origin_form->params().find("grep");
  if (params_grep != origin_form->params().end()) {
    result.maybe_grep = (*params_grep).value;
  }

  return result;
}

http::message_generator Handler::handle_request_(
    http::request<http::string_body>&& req) {
  // Only accept HTTP GET verb
  if (req.method() != http::verb::get)
    return bad_request(req, "Unsupported HTTP verb");

  auto maybe_request = parse_log_request(req.target());
  if (!maybe_request) {
    return bad_request(req, "Invalid request");
  }
  auto& request = *maybe_request;

  auto full_file_path =
      root_dir_ / std::filesystem::path(request.file_path).relative_path();

  spdlog::trace("Going to open the file at {}", full_file_path.string());

  auto log_stream = open_file(
      full_file_path, request.maybe_n.value_or(10), request.maybe_grep);
  if (!log_stream) {
    return not_found(req);
  }

  http::response<LogBody> res{http::status::ok, req.version()};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/plain");
  res.keep_alive(req.keep_alive());
  res.body() = std::move(log_stream);
  res.prepare_payload();
  return res;
}

std::unique_ptr<LogStream> Handler::open_file(std::filesystem::path path,
    size_t n, std::optional<std::string> maybe_grep) {
  if (!std::filesystem::is_regular_file(path)) {
    return nullptr;
  }
  auto result = std::make_unique<LogStream>();
  result->input_stream.open(path);
  if (!result->input_stream.is_open() || !result->input_stream.good()) {
    return nullptr;
  }
  if (maybe_grep) {
    result->grep = [grep = *maybe_grep](
                       std::string_view sv) { return sv.contains(grep); };
  } else {
    result->grep = [](std::string_view) { return true; };
  }
  result->generator = tail(result->input_stream, n);

  return result;
}
