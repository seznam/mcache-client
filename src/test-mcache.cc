/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache.
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
 *       2012-09-16 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <algorithm>

#include <mcache/mcache.h>

class fake_protobuf_t {
public:
    bool SerializeToString(std::string *result) const {
        *result = value;
        return true;
    }
    bool ParseFromString(const std::string &data) {
        value = data;
        return true;
    }
    bool IsInitialized() const { return true;}

    std::string value;
};

class frpc_value_t {
public:
    std::string marshallToFRPCBinaryStream() const { return "";}
};

class frpc_pool_t {
public:
    const frpc_value_t &
    demarshallFromFRPCBinaryStream(const std::string &) {
        return *(new frpc_value_t());
    }
};

class bad_t {
public:
};

int main(int argc, char **argv) {
    mc::init();

    // params
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " server:port" << std::endl;
        return EXIT_FAILURE;
    }

    // prepare client params
    std::vector<std::string> servers;
    std::copy(argv + 1, argv + argc, std::back_inserter(servers));

    // client
    //mc::thread::client_t client(servers);
    mc::client_template_t<
        mc::thread::pool_t,
        mc::thread::server_proxies_t,
        mc::proto::txt::api
    > client(servers);

    try {

        client.set("three", (char)3);
        client.set("three", (wchar_t)3);
        client.set("three", (unsigned wchar_t)3);
        client.set("three", (unsigned char)3);
        client.set("three", (signed int)3);
        client.set("three", (bool)3);
        client.set("three", (int8_t)3);
        client.set("three", (int16_t)3);
        client.set("three", (int32_t)3);
        client.set("three", (int64_t)3);
        client.set("three", (uint8_t)3);
        client.set("three", (uint16_t)3);
        client.set("three", (uint32_t)3);
        client.set("three", (uint64_t)3);

        client.set("three", (unsigned long long int)3);
        client.set("three", (long long int)3);

        client.set("three", (float)3);
        client.set("three", (double)3);
        client.set("three", (long double)3);

        fake_protobuf_t protobuf;
        protobuf.value = "three";
        client.set("three", protobuf);
        client.get("three").as<fake_protobuf_t>();

        client.get("three").as<std::string>();

        frpc_pool_t pool;
        frpc_value_t fvalue;
        client.set("three", fvalue);
        client.get("three").as<frpc_value_t>(pool);

        //bad_t bad;
        //client.set("three", bad);

        char x[3] = {'b', 'b', '\0'};
        client.set("three", x);

        volatile int prdel = 3;
        client.set("three", prdel);

        const int prdel1 = 3;
        client.set("three", prdel1);

        client.add("three", "3");
        client.add("tyhree", "3");

        client.prepend("three", "3");
        client.prepend("txhree", "3");
        client.append("three", "3");
        client.append("txhree", "3");

        mc::result_t res = client.gets("three");
        client.del("txhree");
        client.cas("txhree", "3", mc::opts_t(0, 0, res.cas));
        try {
            client.cas("three", "3", mc::opts_t(0, 0, res.cas - 1));
            throw 3;
        } catch (const mc::proto::error_t &e) {
            if (e.code() != mc::proto::resp::exists) throw;
        }

        client.incr("three");
        client.decr("three");

        client.del("three");
        client.del("tzhree");

        if (client.get("three")) throw 3;
        client.get("aaaa").as<int>();
        //std::cout << client.dump() << std::endl;

        {
            mc::result_t res = client.flush_all(3);
            if (!res) std::cout << res.data << std::endl;
        }

    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;

    } catch (...) {
        std::cerr << "ERROR: unknown exception" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

