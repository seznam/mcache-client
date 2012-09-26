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

#include <string>
#include <inttypes.h>

#include <mcache/error.h>

namespace mc {
namespace proto {
namespace resp {

/** Server response codes.
 */
enum response_code_t {
    ok           = 200,
    stored       = 201,
    deleted      = 202,
    touched      = 203,

    not_stored   = 400,
    exists       = 401,
    not_found    = 404,

    error        = 500,
    client_error = 501,
    server_error = 502,
    empty        = 503,
    io_error     = 504,

    unrecognized = 1000,
};

} // namespace resp

/** Memcache server response holder.
 */
class single_response_t {
public:
    /** C'tor.
     */
    explicit single_response_t(int status,
                               const std::string &data = std::string())
        : status(status), data(data)
    {}

    /** Returns true if not error occurred.
     */
    inline operator bool() const { return (status / 100) == 2;}

    /** Creates new exceptions value from response code and response message.
     */
    inline error_t exception() const { return error_t(status, data);}

    /** Returns status code.
     */
    inline int code() const { return status;}

protected:
    int status;       //!< status code
    std::string data; //!< response body/message
};

/** Response for single retrieval commands.
 */
class single_retrival_response_t: public single_response_t {
public:
    /** C'tor.
     */
    single_retrival_response_t(int status, const std::string &data)
        : single_response_t(status, data),
          flags(), cas(), bytes()
    {}

    /** C'tor.
     */
    single_retrival_response_t(uint32_t flags, std::size_t bytes, uint64_t cas)
        : single_response_t(resp::ok, std::string()),
          flags(flags), cas(cas), bytes(bytes + foother_size)
    {}

    /** C'tor.
     */
    explicit single_retrival_response_t(const single_response_t &resp)
        : single_response_t(resp),
          flags(), cas(), bytes()
    {}

    // mark that this response expects body
    class body_tag;

    /** Returns retrieval commands response data.
     */
    inline const std::string &body() const { return data;}

    /** Sets new retrieval commands body response.
     */
    inline void set_body(const std::string &body) {
        data = body;
        data.resize(data.size() - foother_size);
    }

    /** Retutns retrieval commands expected body size.
     */
    inline std::size_t expected_body_size() const { return bytes;}

    const uint32_t flags; //!< retrieval commands response flags
    const uint64_t cas;   //!< retrieval commands cas attribute

protected:
    std::size_t bytes;    //!< expected response body size
    static const std::size_t foother_size = 7; //!< sizeof("\r\nEND\r\n")
};

// TODO(burlog): add support for multi-get
// here should be vector of single and api for geting data by key

} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_RESPONSE_H */

