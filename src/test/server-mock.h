/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Server mock object.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-17 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_SRC_TEST_SERVER_MOCK_H
#define MCACHE_SRC_TEST_SERVER_MOCK_H

#include <map>
#include <string>

#include "error.h"
#include "mcache/client.h"

namespace mc {
namespace mock {

/** Server mock object.
 */
class server_t {
public:
    template <typename command_t>
    response_t send(const command_t &command) const {
        DBG(DBG1, "MC::MOCK: call: server=%s, command=%s, key=%s",
                  addr.c_str(), command.name(), command.key().c_str());

        if (command.name() == std::string("get")) {
            std::string key = command.key();
            std::map<std::string, std::string>::const_iterator
                ivalue = storage.find(key);
            if (ivalue == storage.end()) return response_t("");
            return response_t(ivalue->second);

        } else if (command.name() == std::string("set")) {
            std::string key = command.key();
            storage[key] = command.data();
            return response_t("");

        }
        throw std::runtime_error("unknown command");
    }

    bool is_dead() const { return false;}

    server_t(const std::string &addr): addr(addr) {}

    mutable std::map<std::string, std::string> storage;
    std::string addr;
};

} // namespace mock
} // namespace mc

#endif /* MCACHE_SRC_TEST_SERVER_MOCK_H */



