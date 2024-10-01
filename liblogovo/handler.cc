#include "handler.h"

#include <spdlog/spdlog.h>

#include <boost/beast/version.hpp>
#include <boost/url/parse.hpp>
#include <fstream>

#include "tail.h"
#include "vendor/generator.h"

namespace beast = boost::beast;
namespace http = beast::http;

// Amount of lines to send if no number was explicitly requested.
constexpr size_t DEFAULT_N = 10;
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

// Data required for serving a single log get request. Used as a value_type for
// LogBody
struct LogStream {
  // generator holds a reference to the stream, so we store it right in this
  // structure to ensure it's alive for the whole duration of the request.
  std::ifstream input_stream;
  std::generator<std::string_view> generator;
};

// Boost Beast body writer that fetches data from the LogStream and feeds it to
// the network without having to save it in some sort of buffer first.
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
    try {
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
      auto log_line = *current;
      // Sending lines one at a time might not be the most efficient thing, so
      // an obvious way to improve performance is to add buffering here at the
      // writer.
      auto result_buffer =
          beast::net::const_buffer(log_line.data(), log_line.size());

      return std::make_pair(result_buffer, true);
    } catch (const std::exception& e) {
      spdlog::error(e.what());
      return boost::none;
    }
  }

 private:
  std::optional<std::generator<std::string_view>::iterator> maybe_current_;
  LogStream* log_stream_;
};

// Boost Beast body type. We aren't using any of the default body types because
// we want to stream data from the `tail` generator right as we fetch it instead
// of saving it to some kind of buffer.
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

  // We're converting path to lexically normal form here to prevent trickery
  // like ./../../<something>
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

  auto log_stream = make_log_stream(
      full_file_path, request.maybe_n.value_or(DEFAULT_N), request.maybe_grep);
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

std::unique_ptr<LogStream> Handler::make_log_stream(std::filesystem::path path,
    size_t n, std::optional<std::string> maybe_grep) {
  if (!std::filesystem::is_regular_file(path)) {
    return nullptr;
  }
  auto result = std::make_unique<LogStream>();
  result->input_stream.open(path);
  if (!result->input_stream.is_open() || !result->input_stream.good()) {
    return nullptr;
  }
  result->generator = tail(result->input_stream, n, maybe_grep);

  return result;
}
