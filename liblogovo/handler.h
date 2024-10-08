#pragma once

#include <boost/beast/http.hpp>
#include <filesystem>

class LogStream;

class Handler {
 public:
  Handler(std::filesystem::path root_dir);

  boost::beast::http::message_generator handle_request(
      boost::beast::http::request<boost::beast::http::string_body>&& req);

 private:
  boost::beast::http::message_generator handle_request_(
      boost::beast::http::request<boost::beast::http::string_body>&& req);

  std::unique_ptr<LogStream> make_log_stream(
      std::filesystem::path, size_t n, std::optional<std::string> maybe_grep);

  std::filesystem::path root_dir_;
};
