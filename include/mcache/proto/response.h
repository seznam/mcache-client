/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: response holder.
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

#ifndef MCACHE_PROTO_RESPONSE_H
#define MCACHE_PROTO_RESPONSE_H

namespace mc {

/** Memcache server response holder.
 */
class response_t {
public:
    operator bool() const { return true;}
    int exception() const { return 3;}

    response_t(const std::string &data): data(data) {}

    int error;        //!< error code
    std::string data;
};

} // namespace mc

#endif /* MCACHE_PROTO_RESPONSE_H */

