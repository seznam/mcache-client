/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: storage
 *                  command options.
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

#ifndef MCACHE_PROTO_OPTS_H
#define MCACHE_PROTO_OPTS_H

#include <string>
#include <inttypes.h>

namespace mc {
namespace proto {

/** Storage commands options.
 *
 * * <flags> is an arbitrary 16-bit unsigned integer (written out in decimal)
 * that the server stores along with the data and sends back when the item is
 * retrieved. Clients may use this as a bit field to store data-specific
 * information; this field is opaque to the server.  Note that in memcached
 * 1.2.1 and higher, flags may be 32-bits, instead of 16, but you might want to
 * restrict yourself to 16 bits for compatibility with older versions.
 *
 *  * <exptime> is expiration time. If it's 0, the item never expires (although
 *  it may be deleted from the cache to make place for other items). If it's
 *  non-zero (either Unix time or offset in seconds from current time), it is
 *  guaranteed that clients will not be able to retrieve this item after the
 *  expiration time arrives (measured by server time).
 *
 *  The actual value sent as <exptime> may either be Unix time (number of
 *  seconds since January 1, 1970, as a 32-bit value), or a number of seconds
 *  starting from current time. In the latter case, this number of seconds may
 *  not exceed 60*60*24*30 (number of seconds in 30 days); if the number sent
 *  by a client is larger than that, the server will consider it to be real
 *  Unix time value rather than an offset from current time.
 *
 *  * <cas> is a check and set operation which means "store this data but only
 *  if no one else has updated since I last fetched it." And its value is a
 *  unique 64-bit value of an existing entry.  Clients should use the value
 *  returned from the "gets" command when issuing "cas" updates.
 */
class opts_t {
public:
    /** C'tor.
     */
    opts_t(time_t expiration = 0, uint32_t flags = 0, uint64_t cas = 0)
        : expiration(expiration), flags(flags), cas(cas)
    {}

    const time_t expiration; //!< expiration time (seconds from now at server)
    const uint32_t flags;    //!< flags for held value on server
    union {
        const uint64_t cas;      //!< unique identifier retrieved from
                                 //!gets command
        uint64_t initial;        //!< The initial value for
                                 //!increment/decrement
    };
};

} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_OPTS_H */

