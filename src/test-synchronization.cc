/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: synchronization primitives test
 *                  case.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-10-02 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <sys/types.h>
#include <sys/wait.h>

#include <mcache/lock.h>
#include <mcache/error.h>
#include <mcache/io/error.h>
#include <mcache/server-proxy.h>
#include <mcache/server-proxies.h>

namespace test {

class fake_command_t {
public:
    typedef mc::proto::single_response_t response_t;
    std::string serialize() const { return std::string();}
    std::string header_delimiter() const { return std::string();}
    response_t deserialize_header(const std::string &) const {
        return response_t(mc::proto::resp::error);
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
    connections_t(const std::string &, const mc::io::opts_t &) {}
    connection_ptr_t pick() { return connection_ptr_t(new connection_t());}
    void push_back(connection_ptr_t) {}
    void clear() {}
};

bool sharing_dead_server_thread() {
    std::cout << __PRETTY_FUNCTION__ << ": " << std::flush;

    // prepare
    using mc::thread::lock_t;
    using mc::thread::shared_array_t;
    typedef connections_t<always_fail_connection_t> connections_t;
    typedef mc::server_proxy_t<lock_t, connections_t> server_proxy_t;
    typedef mc::server_proxies_t<shared_array_t, server_proxy_t> proxies_t;
    std::vector<std::string> servers;
    servers.push_back("server1:11211");
    servers.push_back("server2:11211");
    servers.push_back("server3:11211");
    mc::server_proxy_config_t cfg;
    proxies_t proxies(servers, cfg);

    // run
    boost::thread_group threads;
    threads.create_thread(boost::bind(&server_proxy_t::send<fake_command_t>,
                                      &proxies[1], fake_command_t()));
    threads.join_all();
    return proxies[1].is_dead();
}

void sharing_lock_thread_helper(mc::thread::lock_t *lock) {
    mc::scope_guard_t<mc::thread::lock_t> guard(*lock);
    do {} while (!guard.try_lock());
    ::sleep(2);
}

bool sharing_lock_thread() {
    std::cout << __PRETTY_FUNCTION__ << ": " << std::flush;

    // run
    mc::thread::lock_t lock;
    boost::thread_group threads;
    threads.create_thread(boost::bind(&sharing_lock_thread_helper, &lock));
    ::sleep(1);
    bool result = !lock.try_lock();
    threads.join_all();
    return result;
}

bool sharing_dead_server_ipc() {
    std::cout << __PRETTY_FUNCTION__ << ": " << std::flush;

    // prepare
    using mc::ipc::lock_t;
    using mc::ipc::shared_array_t;
    typedef connections_t<always_fail_connection_t> connections_t;
    typedef mc::server_proxy_t<lock_t, connections_t> server_proxy_t;
    typedef mc::server_proxies_t<shared_array_t, server_proxy_t> proxies_t;
    std::vector<std::string> servers;
    servers.push_back("server1:11211");
    servers.push_back("server2:11211");
    servers.push_back("server3:11211");
    mc::server_proxy_config_t cfg;
    proxies_t proxies(servers, cfg);

    // run
    pid_t pid = ::fork();
    if (pid) {
        // error
        if (pid < 0) return false;

        // parent
        int status = 0;
        if (::wait(&status) != pid) return false;
        if (WEXITSTATUS(status)) return false;

    } else {
        // child
        try {
            proxies[1].send(fake_command_t());
            ::exit(0);
        } catch (const std::exception &) {}
        ::exit(1);
    }
    return proxies[1].is_dead();
}

bool sharing_lock_ipc() {
    std::cout << __PRETTY_FUNCTION__ << ": " << std::flush;

    // run
    mc::ipc::shared_array_t<mc::ipc::lock_t> locks(1);
    mc::ipc::lock_t &lock = locks[0];
    pid_t pid = ::fork();
    if (pid) {
        // error
        if (pid < 0) return false;

        // parent
        ::sleep(1);
        bool result = !lock.try_lock();
        int status = 0;
        if (::wait(&status) != pid) return false;
        if (WEXITSTATUS(status)) return false;
        return result;

    } else {
        // child
        {
            mc::scope_guard_t<mc::ipc::lock_t> guard(lock);
            do {} while (!guard.try_lock());
            ::sleep(2);
        }
        ::exit(0);
    }
    return false;
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
    test::Checker_t check;
    check(test::sharing_dead_server_thread());
    check(test::sharing_lock_thread());
    check(test::sharing_dead_server_ipc());
    check(test::sharing_lock_ipc());
    return check.fails;
}

