/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-16 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/lambda/lambda.hpp>

#include <mcache/init.h>
#include <mcache/error.h>
#include <mcache/client.h>
#include <mcache/pool/consistent-hashing.h>
#include <mcache/pool/mod.h>

#include <test/server-mock.h>

#include <vector>
#include <mcache/hash.h>

//int main(int argc, char **argv) {
int main(int, char **) {
    mc::init();
    using boost::lambda::_1;
    ////uint32_t (*hash1)(const std::string &) = mc::city;
    //uint32_t (*hash2)(const std::string &, uint32_t) = mc::city;

    std::vector<std::string> s;
    s.push_back("aaaaaaaaa:1");
    s.push_back("bbbbbbbbb:2");
    s.push_back("ccccccccc:3");
    s.push_back("ddddddddd:4");
    s.push_back("e:5");
    s.push_back("f:6");
    s.push_back("g:7");
    s.push_back("h:8");

    //// client
    //typedef mc::client_template_t<
    //            mc::hash_function_t,
    //            mc::consistent_hashing_pool_t,
    //            mc::mock::server_t
    //        > client_t;
    //mc::consistent_hashing_pool_t pool(s, hash2, 1000);
    //client_t client(mc::city, pool, s);

    //client.set("a", "b");
    //std::cout << client.get("a") << std::endl;

    //typedef mc::mod_pool_t pool_t;
    //mc::mod_pool_t pool(s);
    //typedef mc::consistent_hashing_pool_t pool_t;
    //std::cout << pool.dump() << std::endl;

    //std::cout << "Test: " << std::endl
    //          << "key=" << "ahoj"
    //          << ", hash=" << mc::city("ahoj")
    //          << ", server=" << *pool.choose(mc::city("ahoj")) << std::endl;

    //::srand(static_cast<uint32_t>(::time(0x0)));

    //std::vector<uint32_t> S(8, 0);

    //for (uint32_t i = 0; i < 10000000; ++i) {
    //    std::string x;
    //    uint32_t l = ::rand() % 26 + 1;
    //    x.reserve(l);
    //    for (unsigned int j = 0; j < l; ++j) x.append(1, 'a' + (::rand() % 26));

    //    for (pool_t::const_iterator
    //            iidx = pool.choose(mc::city(x)),
    //            eidx = pool.end();
    //            iidx != eidx; ++iidx)
    //    {
    //        ++S[*iidx];
    //        break;
    //    //response_t response = iserver->send(command.str());
    //    //if (response) return response;
    //    }
    //}

    //std::for_each(S.begin(), S.end(), std::cout << _1 << '\n');
    ////throw error_t("out of servers");

}

