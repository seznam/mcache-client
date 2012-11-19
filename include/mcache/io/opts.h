/*
 * FILE             $Id: $
 *
 * DESCRIPTION      I/O objects options.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-26 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_IO_OPTS_H
#define MCACHE_IO_OPTS_H

#include <inttypes.h>

namespace mc {
namespace io {

/** Connection options.
 */
class opts_t {
public:
    /** Timeouts collection.
     */
    class timeouts_t {
    public:
        /** C'tor.
         */
        timeouts_t(uint64_t connect = 500,
                   uint64_t read = 1000,
                   uint64_t write = 1000)
            : connect(connect), read(read), write(write)
        {}

        uint64_t connect; //!< timeout to connect
        uint64_t read;    //!< timeout to single read op
        uint64_t write;   //!< timeout to single write op
    };

    /** C'tor.
     */
    opts_t(): timeouts(), max_connections_in_pool(30) {}

    /** C'tor.
     */
    opts_t(uint64_t connect, uint64_t read, uint64_t write)
        : timeouts(connect, read, write)
    {}

    timeouts_t timeouts;              //!< connection timeouts
    uint64_t max_connections_in_pool; //!< max count of connections in pool
};

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_OPTS_H */

