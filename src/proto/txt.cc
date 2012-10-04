/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: text protocol
 *                  implementation base class.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-24 (bukovsky)
 *                  First draft.
 */

#include <sstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "error.h"
#include "mcache/error.h"
#include "mcache/proto/txt.h"

// for protocol details:
// @see https://github.com/memcached/memcached/blob/master/doc/protocol.txt

namespace mc {
namespace proto {
namespace txt {
namespace {

/** Extracts error description from response header.
 */
std::string error_desc(const std::string &header) {
    std::string::size_type space = header.find(' ');
    if (space == std::string::npos) return std::string();
    return boost::trim_copy(header.substr(space));
}

/** Parse VALUE response of retrieval commands.
 *
 *  VALUE <key> <flags> <bytes> [<cas unique>]\r\n
 */
retrieve_command_t::response_t
deserialize_value_resp(const std::string &header) {
    // prepare response storage
    typedef retrieve_command_t::response_t response_t;
    std::string unused;
    uint32_t flags = 0;
    std::size_t bytes = 0;
    uint64_t cas = 0;
    DBG(DBG1, "Parsing header of get command: line=%s",
              log::escape(header).c_str());

    // parse and return
    std::istringstream is(header);
    if ((is >> unused) && (is >> unused) && (is >> flags) && (is >> bytes)) {
        is >> cas;
        return response_t(flags, bytes, cas, retrieve_command_t::footer_size);
    }
    return response_t(resp::syntax, "invalid response: " + header);
}

} // namespace

command_t::response_t
command_t::deserialize_header(const std::string &header) const {
    // try parse general errors (my childs ensures that header is not empty)
    switch (header[0]) {
    case 'E':
        if (boost::starts_with(header, "ERROR"))
            return response_t(resp::error, "syntax error");
        break;
    case 'C':
        if (boost::starts_with(header, "CLIENT_ERROR"))
            return response_t(resp::client_error, error_desc(header));
        break;
    case 'S':
        if (boost::starts_with(header, "SERVER_ERROR"))
            return response_t(resp::server_error, error_desc(header));
        break;
    default: break;
    }

    // header does not recognized, return unrecognized
    return response_t(resp::unrecognized, header);
}

retrieve_command_t::response_t
retrieve_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // try parse retrieve responses
    switch (header[0]) {
    case 'E':
        if (boost::starts_with(header, "END"))
            return response_t(resp::not_found, "not found");
        break;
    case 'V':
        if (boost::starts_with(header, "VALUE"))
            return deserialize_value_resp(header);
        break;
    default: break;
    }

    // header does not recognized, try global errors
    return response_t(command_t::deserialize_header(header));
}

std::string retrieve_command_t::serialize(const char *name) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    std::string result;
    result.append(name).append(1, ' ').append(key).append(header_delimiter());
    return result;
}

storage_command_t::response_t
storage_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // try parse retrieve responses
    switch (header[0]) {
    case 'E':
        if (boost::starts_with(header, "EXISTS"))
            return response_t(resp::exists, "cas id expired");
        break;
    case 'N':
        if (boost::starts_with(header, "NOT_FOUND"))
            return response_t(resp::not_found, "cas id is invalid");
        if (boost::starts_with(header, "NOT_STORED"))
            return response_t(resp::not_stored, "key (does not) exist");
        break;
    case 'S':
        if (boost::starts_with(header, "STORED"))
            return response_t(resp::stored);
        break;
    default: break;
    }

    // header does not recognized, try global errors
    return command_t::deserialize_header(header);
}

std::string storage_command_t::serialize(const char *name) const {
    // <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n
    // cas <key> <flags> <exptime> <bytes> <cas unique> [noreply]\r\n
    // TODO(burlog): check whether key fulfil protocol constraints
    std::ostringstream os;
    os << name << ' '
       << key << ' '
       << opts.flags << ' '
       << opts.expiration << ' '
       << data.size();
    if (opts.cas) os << ' ' << opts.cas;
    os << header_delimiter() << data << header_delimiter();
    return os.str();
}

incr_decr_command_t::response_t
incr_decr_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // try parse retrieve responses
    switch (header[0]) {
    case 'T':
        // fail response
        if (boost::starts_with(header, "TOUCHED"))
            return response_t(resp::touched);
        break;
    case 'N':
        if (boost::starts_with(header, "NOT_FOUND"))
            return response_t(resp::not_found, "key does not exist");
        break;
    case '0'...'9': {
        // <value>\r\n - ok response
        std::istringstream is(header);
        uint64_t value;
        if (is >> value)
            return response_t(resp::ok, boost::trim_copy(header));
        break;
        }
    default: break;
    }

    // header does not recognized, try global errors
    return command_t::deserialize_header(header);
}

std::string incr_decr_command_t::serialize(const char *name) const {
    // <command name> <key> <value> [noreply]\r\n
    // TODO(burlog): check whether key fulfil protocol constraints
    std::ostringstream os;
    os << name << ' ' << key << ' ' << value << header_delimiter();
    return os.str();
}

delete_command_t::response_t
delete_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // try parse retrieve responses
    switch (header[0]) {
    case 'D':
        if (boost::starts_with(header, "DELETED"))
            return response_t(resp::deleted);
        break;
    case 'N':
        if (boost::starts_with(header, "NOT_FOUND"))
            return response_t(resp::not_found, "key does not exist");
        break;
    default: break;
    }

    // header does not recognized, try global errors
    return response_t(command_t::deserialize_header(header));

}

std::string delete_command_t::serialize() const {
    std::string result;
    result.append("delete ").append(key).append(header_delimiter());
    return result;
}

// command names
const char *api::get_name = "get";
const char *api::gets_name = "gets";
const char *api::set_name = "set";
const char *api::add_name = "add";
const char *api::replace_name = "replace";
const char *api::append_name = "append";
const char *api::prepend_name = "prepend";
const char *api::cas_name = "cas";
const char *api::incr_name = "incr";
const char *api::decr_name = "decr";
const char *api::touch_name = "touch";

} // namespace txt
} // namespace proto
} // namespace mc

