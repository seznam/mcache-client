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
    std::copy(argv + 1, argv + argc - 1, std::back_inserter(servers));

    // client
    mc::thread::client_t client;
    client.set("three", "3");
    std::cout << client.get("three") << std::endl;
    return EXIT_SUCCESS;
}

