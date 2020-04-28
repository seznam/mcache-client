/*
 * FILE             $Id: $
 *
 * DESCRIPTION      I/O objects options.
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
 *       2012-09-26 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_IO_OPTS_H
#define MCACHE_IO_OPTS_H

#include <mcache/time-units.h>

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
        timeouts_t()
            : connect(500ms), read(1000ms), write(1000ms)
        {}

        /** C'tor.
         */
        [[deprecated("convert agrs to std::chrono::milliseconds")]]
        timeouts_t(uint64_t connect)
            : connect(connect), read(1000ms), write(1000ms)
        {}

        /** C'tor.
         */
        [[deprecated("convert agrs to std::chrono::milliseconds")]]
        timeouts_t(uint64_t connect, uint64_t read)
            : connect(connect), read(read), write(1000ms)
        {}

        /** C'tor.
         */
        [[deprecated("convert agrs to std::chrono::milliseconds")]]
        timeouts_t(uint64_t connect, uint64_t read, uint64_t write)
            : connect(connect), read(read), write(write)
        {}

        /** C'tor.
         */
        timeouts_t(milliseconds_t connect)
            : connect(connect), read(1000ms), write(1000ms)
        {}

        /** C'tor.
         */
        timeouts_t(milliseconds_t connect, milliseconds_t read)
            : connect(connect), read(read), write(1000ms)
        {}

        /** C'tor.
         */
        timeouts_t(milliseconds_t connect,
                   milliseconds_t read,
                   milliseconds_t write)
            : connect(connect), read(read), write(write)
        {}

        milliseconds_t connect; //!< timeout to connect
        milliseconds_t read;    //!< timeout to single read op
        milliseconds_t write;   //!< timeout to single write op
    };

    /** C'tor.
     */
    opts_t(): timeouts(), max_connections_in_pool(30) {}

    /** C'tor.
     */
    [[deprecated("convert agrs to std::chrono::milliseconds")]]
    opts_t(uint64_t connect,
           uint64_t read,
           uint64_t write,
           uint64_t max_connections_in_pool = 30)
        : timeouts(milliseconds_t(connect),
                   milliseconds_t(read),
                   milliseconds_t(write)),
          max_connections_in_pool(max_connections_in_pool)
    {}

    /** C'tor.
     */
    opts_t(milliseconds_t connect,
           milliseconds_t read,
           milliseconds_t write,
           uint64_t max_connections_in_pool = 30)
        : timeouts(connect, read, write),
          max_connections_in_pool(max_connections_in_pool)
    {}

    timeouts_t timeouts;              //!< connection timeouts
    uint64_t max_connections_in_pool; //!< max count of connections in pool
};

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_OPTS_H */

