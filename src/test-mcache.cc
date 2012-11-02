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
#include <vector>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/lambda/lambda.hpp>

#include <mcache/mcache.h>

int main(int argc, char **argv) {
    mc::init();
    using boost::lambda::_1;

    // params
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " server:port" << std::endl;
        return EXIT_FAILURE;
    }

    // prepare client params
    std::vector<std::string> servers;
    std::copy(argv + 1, argv + argc, std::back_inserter(servers));

    // client
    mc::thread::pool_config_t pcfg;
    mc::thread::server_proxy_config_t scfg;
    mc::thread::client_t client(servers, pcfg, scfg);
    //std::cout << client.pool.dump() << std::endl;
    client.set("three", "3");

    client.add("three", "3");
    client.add("tyhree", "3");

    client.prepend("three", "3");
    client.prepend("txhree", "3");
    client.append("three", "3");
    client.append("txhree", "3");

    mc::result_t res = client.gets("three");
    client.cas("txhree", "3", res.cas);
    client.cas("three", "3", res.cas);
    try {
        client.cas("three", "3", res.cas - 1);
        throw 3;
    } catch (const mc::proto::error_t &e) {
        if (e.code() != mc::proto::resp::exists) throw;
    }

    client.incr("three");
    client.decr("three");

    client.del("three");
    client.del("tzhree");

    if (client.get("three")) throw 3;
    //std::cout << client.dump() << std::endl;
    return EXIT_SUCCESS;
}

