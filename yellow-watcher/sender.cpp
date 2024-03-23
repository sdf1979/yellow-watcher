// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "sender.h"
#include <iostream>

static auto LOGGER = Logger::getInstance();

using namespace std;

void fail(const beast::error_code& ec, char const* what) {
    string msg("sender: ");
    msg.append(what).append(": ").append(ec.message());
    LOGGER->Print(WideCharToUtf8(AnsiToWideChar(msg)), Logger::Type::Error);
}

class Session : public enable_shared_from_this<Session> {
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    string host_;
    string port_;
    string login_;
    string password_;
public:
    explicit Session(net::io_context& ioc, const string& host, const string& port, const string& password, const string& login);
    void Run(char const* target, const string& data);
    void OnResolve(const beast::error_code& ec, tcp::resolver::results_type results);
    void OnConnect(const beast::error_code& ec, tcp::resolver::results_type::endpoint_type);
    void OnWrite(const beast::error_code& ec, std::size_t bytes_transferred);
    void OnRead(beast::error_code ec, std::size_t bytes_transferred);
};

Sender::Sender(const string& host, const string& port, const string& login, const string& password) :
    host_(host),
    port_(port),
    login_(login),
    password_(password)
{}

void Sender::Send(const string& target, const string& data) {
    net::io_context ioc;
    shared_ptr<Session> session = make_shared<Session>(ioc, host_, port_, login_, password_);
    session->Run(target.c_str(), data);
    ioc.run();
}

Session::Session(net::io_context& ioc, const string& host, const string& port, const string& login, const string& password) :
    resolver_(net::make_strand(ioc)),
    stream_(net::make_strand(ioc)),
    host_(host),
    port_(port),
    login_(login),
    password_(password)
{}

void Session::Run(char const* target, const string& data) {
    string auth = login_;
    auth.append(":")
    .append(password_);

    string auth_base64;
    auth_base64.resize(base64::encoded_size(auth.size()));
    base64::encode(static_cast<void*>(&auth_base64[0]), static_cast<void*>(&auth[0]), auth.size());
    auth_base64 = "Basic " + auth_base64;

    req_.version(11);
    req_.method(http::verb::post);
    req_.target(target);
    req_.set(http::field::host, host_);
    req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req_.set(http::field::authorization, auth_base64);
    req_.body() = data;
    req_.prepare_payload();

    resolver_.async_resolve(host_, port_, beast::bind_front_handler(&Session::OnResolve, shared_from_this()));
}

void Session::OnResolve(const beast::error_code& ec, tcp::resolver::results_type results) { //-V813
    if (ec) {
        return fail(ec, "resolve");
    }
    stream_.expires_after(std::chrono::seconds(5));
    stream_.async_connect(results, beast::bind_front_handler(&Session::OnConnect, shared_from_this()));
}

void Session::OnConnect(const beast::error_code& ec, tcp::resolver::results_type::endpoint_type) {
    if (ec) {
        return fail(ec, "connect");
    }

    stream_.expires_after(std::chrono::seconds(5));
    http::async_write(stream_, req_, beast::bind_front_handler(&Session::OnWrite, shared_from_this()));
}

void Session::OnWrite(const beast::error_code& ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        return fail(ec, "write");
    }

    http::async_read(stream_, buffer_, res_, beast::bind_front_handler(&Session::OnRead, shared_from_this()));
}

void Session::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        return fail(ec, "read");
    }
    auto res_int = res_.result_int();
    if (res_int != 200) {
        std::string msg = "Send via http. Response code received ";
        msg.append(std::to_string(res_int)).append(".");
        if (!res_.body().empty()) {
            msg.append(" ").append(res_.body());
        }
        LOGGER->Print(msg, Logger::Type::Error);
    }

    stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

    if (ec && ec != beast::errc::not_connected) {
        return fail(ec, "shutdown");
    }
}