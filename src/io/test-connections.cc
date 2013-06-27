/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: connection pool tests.
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
 *       2012-09-21 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <boost/lambda/lambda.hpp>
#include <boost/shared_ptr.hpp>

#include <mcache/init.h>
#include <mcache/io/error.h>
#include <mcache/io/connections.h>

namespace test {

class connection_t {
public:
    connection_t(const std::string &, const mc::io::opts_t &) {}
};

template <typename connections_t, typename output_iterator_t>
void pick_n(connections_t &connections, output_iterator_t iout, std::size_t n) {
    for (; n != 0; ++iout, --n) *iout = connections.pick();
}

template <typename connections_t, typename input_iterator_t>
void push_back(input_iterator_t ic, input_iterator_t ec,
               connections_t &connections)
{
    for (; ic != ec; ++ic) connections.push_back(*ic);
}

template <typename ptr_t>
connection_t *strip(ptr_t ptr) { return ptr.get();}

typedef mc::io::create_new_connection_pool_t<connection_t> cn_connections_t;
typedef mc::io::single_connection_pool_t<connection_t> s_connections_t;
typedef mc::io::bbt::caching_connection_pool_t<connection_t> tbb_connections_t;
typedef mc::io::lock::caching_connection_pool_t<connection_t> l_connections_t;

template <typename connections_t>
bool connections_get() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // run
    typedef typename connections_t::connection_ptr_t connection_ptr_t;
    try {
        connections_t connections("localhost:11211", mc::io::opts_t());
        connection_ptr_t first = connections.pick();
        connection_t *pfirst = &*first;
        connections.push_back(first);
        if (first) return false;
        if (connections.pick().get() != pfirst) return false;
        return true;

    } catch (const std::exception &) {}
    return false;
}

template <typename connections_t>
bool connections_capacity() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // run
    typedef typename connections_t::connection_ptr_t connection_ptr_t;
    try {
        connections_t connections("localhost:11211", mc::io::opts_t());
        std::vector<connection_ptr_t> tmp;

        // pick first portion of connections from pool and store bare pointers
        std::set<connection_t *> bare1;
        pick_n(connections, std::back_inserter(tmp), 4);
        std::transform(tmp.begin(), tmp.end(),
                       std::inserter(bare1, bare1.begin()),
                       &strip<connection_ptr_t>);
        push_back(tmp.begin(), tmp.end(), connections);
        tmp.clear();

        // pick second portion of connection from pool and store bare pointers
        std::set<connection_t *> bare2;
        pick_n(connections, std::back_inserter(tmp), 3);
        std::transform(tmp.begin(), tmp.end(),
                       std::inserter(bare2, bare2.begin()),
                       &strip<connection_ptr_t>);
        push_back(tmp.begin(), tmp.end(), connections);
        tmp.clear();

        // is bare2 subset of bare1?
        bool result = std::includes(bare1.begin(), bare1.end(),
                                    bare2.begin(), bare2.end());
        return result;

    } catch (const std::exception &) {}
    return false;
}

bool single_connection_multi_get() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    try {
        typedef s_connections_t connections_t;
        connections_t connections("localhost:11211", mc::io::opts_t());
        connections_t::connection_ptr_t ptr = connections.pick();
        connections.pick();
        connections.push_back(ptr);
        connections.pick();
        return true;

    } catch (const std::exception &e) { std::cerr << e.what() << std::endl;}
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
    mc::init();
    test::Checker_t check;
    check(test::connections_get<test::s_connections_t>());
    check(test::connections_get<test::cn_connections_t>());
    check(test::connections_get<test::tbb_connections_t>());
    check(test::connections_get<test::l_connections_t>());
    check(test::connections_capacity<test::tbb_connections_t>());
    check(test::connections_capacity<test::l_connections_t>());
    check(test::single_connection_multi_get());
    return check.fails;
}

