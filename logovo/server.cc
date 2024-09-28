#include "server.h"

#include <spdlog/spdlog.h>

Server::Server(boost::asio::io_context& ioc) : ioc_(ioc) {}

void Server::serve() { spdlog::info("Starting HTTP server"); }
