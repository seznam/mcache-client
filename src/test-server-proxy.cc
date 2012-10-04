/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: server proxy tests.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-21 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/shared_ptr.hpp>

#include <mcache/init.h>
#include <mcache/server-proxy.h>

namespace test {

class fake_command_t {
public:
    typedef mc::proto::single_response_t response_t;
    std::string serialize() const { return std::string();}
    std::string header_delimiter() const { return std::string();}
    response_t deserialize_header(const std::string &) const {
        return response_t(mc::err::internal_error);
    }
};

class always_fail_connection_t {
public:
    template <typename type_t>
    void write(type_t) {
        throw mc::io::error_t(mc::io::err::internal_error, "fake");
    }
    template <typename type_t>
    std::string read(type_t) {
        throw mc::io::error_t(mc::io::err::internal_error, "fake");
    }
};

template <typename connection_t>
class connections_t {
public:
    typedef boost::shared_ptr<connection_t> connection_ptr_t;
    connections_t(const std::string &, uint64_t) {}
    connection_ptr_t pick() { return connection_ptr_t(new connection_t());}
    void push_back(connection_ptr_t) {}
    void reset() {}
};

typedef mc::server_proxy_t<
            mc::none::lock_t,
            connections_t<always_fail_connection_t>
        > server_proxy_t;

bool server_proxy_mark_dead() {
    std::cout << __PRETTY_FUNCTION__ << ": ";
    mc::server_proxy_config_t cfg;
    cfg.restoration_interval = 3;
    cfg.timeout = 1000;
    server_proxy_t::shared_t shared;
    server_proxy_t proxy("server1:11211", &shared, cfg);
    // first call should return true
    if (!proxy.callable()) return false;
    proxy.send(fake_command_t());
    // after bad send server should be dead
    return proxy.is_dead();
}

bool server_proxy_raise_zombie() {
    std::cout << __PRETTY_FUNCTION__ << ": ";
    mc::server_proxy_config_t cfg;
    cfg.restoration_interval = 1;
    cfg.timeout = 1000;
    server_proxy_t::shared_t shared;
    server_proxy_t proxy("server1:11211", &shared, cfg);
    // first call should return true
    if (!proxy.callable()) return false;
    proxy.send(fake_command_t());
    // after bad send shouldn't be callable
    if (proxy.callable()) return false;
    ::sleep(1);
    // after restoration timeout server should be callable
    if (!proxy.callable()) return false;
    // and second should not
    return !proxy.callable();
}

class Checker_t {
public:
    Checker_t(): fails() {}

    void operator()(bool result) {
        fails += !result;
        if (result)
            std::cout << "[01;32m" << "ok" << "[01;0m" << std::endl;
        else
            std::cout << "[01;31m" << "fail" << "[01;0m" << std::endl;
    }

    int fails;
};

} // namespace test

int main(int, char **) {
    mc::init();
    test::Checker_t check;
    check(test::server_proxy_mark_dead());
    check(test::server_proxy_raise_zombie());
    return check.fails;
}

