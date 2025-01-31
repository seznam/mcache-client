/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: consistent hashing pool tests.
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
#include <cxxabi.h>

#include <mcache/init.h>
#include <mcache/error.h>
#include <mcache/pool/consistent-hashing.h>
#include <mcache/pool/mod.h>
#include <mcache/hash.h>

namespace test {

class consistent_hashing_pool_t:
    public mc::consistent_hashing_pool_t<mc::murmur3_t>
{
public:
    consistent_hashing_pool_t(const std::vector<std::string> &addresses)
        : mc::consistent_hashing_pool_t<mc::murmur3_t>(addresses)
    {}

    void remove(const std::string &addr) {
        uint32_t hash = 0;
        for (uint32_t i = 0; i < 200; ++i)
            ring.erase(hash = hashf(addr, hash));
    }
};

std::string rand_string() {
    std::string result;
    uint32_t length = ::rand() % 26 + 1;
    result.reserve(length);
    for (uint32_t i = 0; i < length; ++i)
        result.append(1, 'a' + (::rand() % 26));
    return result;
}

} // namespace test

int main(int, char **) {
    mc::init();

    // prepare servers
    std::vector<std::string> servers;
    servers.push_back("server1:11211");
    servers.push_back("server2:11211");
    servers.push_back("server3:11211");
    servers.push_back("server4:11211");
    servers.push_back("server5:11211");
    servers.push_back("server6:11211");
    servers.push_back("server7:11211");
    servers.push_back("server8:11211");

    // prepare pool and random values storage
    test::consistent_hashing_pool_t pool(servers);
    std::map<std::string, uint32_t> values;
    std::vector<uint32_t> distribution(servers.size(), 0);
    std::vector<uint32_t> migrated(servers.size(), 0);
    std::fill(distribution.begin(), distribution.end(), 0);

    // fill values storage with default setup
    while (values.size() != 10000) {
        std::string key = test::rand_string();
        uint32_t idx = *pool.choose(key);
        if (values.insert(std::make_pair(key, idx)).second)
            ++distribution[idx];
    }
    std::cout << "Distribution of keys over servers: \n";
    for (auto &d: distribution)
        std::cout << d << '\n';

    // remove server3 from pool
    pool.remove("server3:11211");

    // check how many keys got different memcache after one server disappear
    std::fill(distribution.begin(), distribution.end(), 0);
    std::fill(migrated.begin(), migrated.end(), 0);
    uint32_t changed = 0;
    for (std::map<std::string, uint32_t>::const_iterator
            ivalue = values.begin(), evalue = values.end();
            ivalue != evalue; ++ivalue)
    {
        uint32_t idx = *pool.choose(ivalue->first);
        changed += idx != ivalue->second;
        if (idx != ivalue->second) ++migrated[idx];
        ++distribution[idx];
    }
    std::cout << "After removing `server3' "
              << changed << " (" << changed / ((values.size() / 100)) << "%)"
              << " keys shall migrate to other server: \n";
    for (auto &d: distribution)
        std::cout << d << '\n';
    std::cout << "New destinations: \n";
    for (auto &d: distribution)
        std::cout << d << '\n';

    // try add server
    distribution.push_back(0);
    migrated.push_back(0);
    servers.push_back("server9:11211");
    pool = test::consistent_hashing_pool_t(servers);

    // check how many keys got different memcache after one server will be added
    std::fill(distribution.begin(), distribution.end(), 0);
    std::fill(migrated.begin(), migrated.end(), 0);
    changed = 0;
    for (std::map<std::string, uint32_t>::const_iterator
            ivalue = values.begin(), evalue = values.end();
            ivalue != evalue; ++ivalue)
    {
        uint32_t idx = *pool.choose(ivalue->first);
        changed += idx != ivalue->second;
        if (idx != ivalue->second) ++migrated[idx];
        ++distribution[idx];
    }
    std::cout << "After adding `server9' "
              << changed << " (" << changed / ((values.size() / 100)) << "%)"
              << " keys shall migrate to other server: \n";
    for (auto &d: distribution)
        std::cout << d << '\n';
    std::cout << "New destinations: \n";
    for (auto &d: distribution)
        std::cout << d << '\n';

    //std::cout << pool.dump() << std::endl;

    return EXIT_SUCCESS;
}


