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
#include <boost/function.hpp>

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
                               const std::string &aux = std::string())
        : status(status), aux(aux)
    {}

    /** Returns true if not error occurred.
     */
    inline operator bool() const { return (status / 100) == 2;}

    /** Creates new exceptions value from response code and response message.
     */
    inline error_t exception() const { return error_t(status, aux);}

    /** Returns status code.
     */
    inline int code() const { return status;}

    /** Returns error message or other additional data related to status.
     */
    const std::string &data() const { return aux;}

protected:
    int status;      //!< status code
    std::string aux; //!< response body/message
};


/** Response for single retrieval commands.
 */
class single_retrival_response_t: public single_response_t {
public:
    /** Type of callback for setting the body (and flags sometimes). */
    typedef boost::function<void (uint32_t &,
                                  std::string &,
                                  const std::string &)> set_body_callback_t;

    /** C'tor.
     */
    single_retrival_response_t(int status, const std::string &aux)
        : single_response_t(status, aux),
          flags(), cas(), bytes(), set_body_callback(set_body_default)
    {}

    /** C'tor.
     */
    single_retrival_response_t(uint32_t flags, std::size_t bytes, uint64_t cas,
                               set_body_callback_t set_body)
        : single_response_t(resp::ok, std::string()),
          flags(flags), cas(cas), bytes(bytes), set_body_callback(set_body)
    {}

    /** C'tor.
     */
    single_retrival_response_t(int status, std::size_t bytes)
        : single_response_t(status, std::string()),
          flags(), cas(), bytes(bytes),
          set_body_callback(set_body_default)
    {}

    /** C'tor.
     */
    explicit single_retrival_response_t(const single_response_t &resp)
        : single_response_t(resp),
          flags(), cas(), bytes(), set_body_callback(set_body_default)
    {}

    // mark that this response expects body
    class body_tag;

    /** Sets new retrieval commands body response.
     */
    inline void set_body(const std::string &body) {
        set_body_callback(flags, aux, body);
    }

    /** Retutns retrieval commands expected body size.
     */
    inline std::size_t expected_body_size() const { return bytes;}

    uint32_t flags; //!< retrieval commands response flags
    const uint64_t cas;   //!< retrieval commands cas attribute

protected:
    std::size_t bytes;    //!< expected response body size

    /** The callback for setting the body */
    set_body_callback_t set_body_callback;

private:
    /** The default callback for setting the body */
    static void set_body_default(uint32_t &,
                                 std::string &body,
                                 const std::string &data) {
        body = data;
    }
};

// TODO(burlog): add support for multi-get
// here should be vector of single and api for geting data by key

} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_RESPONSE_H */

