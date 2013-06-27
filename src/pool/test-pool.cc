/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: pool tests.
 *
 * PROJECT          Seznam memcache client.
 *
 * LICENSE          See COPYING
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-19 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/if.hpp>
#include <cxxabi.h>

#include <mcache/init.h>
#include <mcache/error.h>
#include <mcache/pool/consistent-hashing.h>
#include <mcache/pool/mod.h>
#include <mcache/hash.h>

namespace test {

class fake_t {
public:
    uint32_t operator()(const std::string &str, uint32_t seed = 0) const {
        if (str == "server1:11211") return seed + 1000;
        if (str == "server2:11211") return seed + 10000;
        if (str == "server3:11211") return seed + 100000;
        if (str == "a") return 333;
        if (str == "b") return 3333;
        if (str == "c") return 33333;
        throw std::runtime_error("invalid key");
    }
};

typedef mc::mod_pool_t<mc::murmur3_t> mod_pool_t;
typedef mc::consistent_hashing_pool_t<mc::murmur3_t> consistent_hashing_pool_t;
typedef mc::consistent_hashing_pool_t<fake_t> fake_consistent_hashing_pool_t;

template <typename pool_t>
bool throws_if_empty_addresses() {
    std::cout << __PRETTY_FUNCTION__ << ": ";
    std::vector<std::string> servers;
    try {
        // check whether empty pool throws
        pool_t p(servers);
    } catch (const std::out_of_range &) { return true;}
    return false;
}

bool mod_pool_iteration() {
    std::cout << __PRETTY_FUNCTION__ << ": ";
    std::vector<std::string> servers;
    servers.push_back("server1:11211");
    servers.push_back("server2:11211");
    servers.push_back("server3:11211");
    mod_pool_t pool(servers);
    // checks whether choose() doesn't return end and if ++ operator returns end
    return pool.choose("b") != pool.end() && ++pool.choose("b") == pool.end();
}

bool consistent_hashing_pool_iteration() {
    std::cout << __PRETTY_FUNCTION__ << ": ";
    using boost::lambda::_1;
    using boost::lambda::var;
    using boost::lambda::if_then;
    mc::consistent_hashing_pool_config_t cfg;
    cfg.virtual_nodes = 20;
    std::vector<std::string> servers;
    servers.push_back("server1:11211");
    servers.push_back("server2:11211");
    servers.push_back("server3:11211");
    consistent_hashing_pool_t pool(servers, cfg);
    // checks if we go through all ring nodes then pool throws
    std::size_t cnt = 0;
    std::size_t cnt1 = 0;
    std::size_t cnt2 = 0;
    std::size_t cnt3 = 0;
    // checks whether servers distribution over the ring
    // and whether we const_iterator walks through all nodes double times
    std::for_each(pool.begin(), pool.end(), ++var(cnt));
    // checks whether distribution ring contains node with zero index 40x
    std::for_each(pool.begin(), pool.end(), if_then(_1 == 0, ++var(cnt1)));
    // dtto
    std::for_each(pool.begin(), pool.end(), if_then(_1 == 1, ++var(cnt2)));
    // dtto
    std::for_each(pool.begin(), pool.end(), if_then(_1 == 2, ++var(cnt3)));
    return (cnt == 120) && (cnt1 == 40) && (cnt2 == 40) && (cnt3 == 40);
}

bool consistent_hashing_pool_distribution() {
    std::cout << __PRETTY_FUNCTION__ << ": ";
    mc::consistent_hashing_pool_config_t cfg;
    cfg.virtual_nodes = 3;
    std::vector<std::string> servers;
    servers.push_back("server1:11211");
    servers.push_back("server2:11211");
    servers.push_back("server3:11211");
    fake_consistent_hashing_pool_t pool(servers, cfg);
    // checks whether key distribution with fake hash functor is like:
    // [1000] -> 0
    // [2000] -> 0
    // [3000] -> 0
    // [10000] -> 1
    // [20000] -> 1
    // [30000] -> 1
    // [100000] -> 2
    // [200000] -> 2
    // [300000] -> 2
    fake_consistent_hashing_pool_t::const_iterator iidx = pool.choose("a");
    if ((*iidx != 0) || (*++iidx != 0) || (*++iidx != 0)) return false;
    iidx = pool.choose("b");
    if ((*iidx != 1) || (*++iidx != 1) || (*++iidx != 1)) return false;
    iidx = pool.choose("c");
    if ((*iidx != 2) || (*++iidx != 2) || (*++iidx != 2)) return false;
    return (*++iidx == 0) && (*pool.choose("server1:11211") == 0);
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
    check(test::throws_if_empty_addresses<test::consistent_hashing_pool_t>());
    check(test::throws_if_empty_addresses<test::mod_pool_t>());
    check(test::mod_pool_iteration());
    check(test::consistent_hashing_pool_iteration());
    check(test::consistent_hashing_pool_distribution());
    return check.fails;
}


