// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <unordered_map>
#include "logger.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace base64 = beast::detail::base64;

using tcp = net::ip::tcp;

class Sender {
    std::string host_;
    std::string port_;
    std::string login_;
    std::string password_;
public:
    Sender(const std::string& host, const std::string& port, const std::string& login, const std::string& password);
    void Send(const std::string& target, const std::string& data);
    void Send(const std::string& target, const std::unordered_map<std::string, std::string>& header, const std::string& data);
    static std::string ToBase64(const std::string& str);
};