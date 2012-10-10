/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: binary protocol
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
 *       2012-09-26 (bukovsky)
 *                  First draft.
 */

#include <endian.h>
#include <sstream>
#include <arpa/inet.h>

#include <boost/lexical_cast.hpp>

#include "error.h"
#include "mcache/proto/binary.h"

namespace mc {
namespace proto {
namespace bin {

// TODO (Lubos) Pokud by se melo pouzivat na solarisu, tak je potreba
// tyhle funkce vytahnout z glibc (htobe atp.
void header_t::prepare_serialization() {
    key_len = htons(key_len);
    reserved = htons(reserved);
    body_len = htonl(body_len);
    opaque = htonl(opaque);
    cas = htobe64(cas);
}

void header_t::prepare_deserialization() {
    key_len = ntohs(key_len);
    reserved = ntohs(reserved);
    body_len = ntohl(body_len);
    opaque = ntohl(opaque);
    cas = be64toh(cas);
}

command_t::response_t
command_t::deserialize_header(const std::string &header) const {
    // // try parse general errors (my childs ensures that header is not empty)
    // switch (header[0]) {
    // case 'E':
    //     if (boost::starts_with(header, "ERROR"))
    //         return response_t(resp::error, "syntax error");
    //     break;
    // case 'C':
    //     if (boost::starts_with(header, "CLIENT_ERROR"))
    //         return response_t(resp::client_error, error_desc(header));
    //     break;
    // case 'S':
    //     if (boost::starts_with(header, "SERVER_ERROR"))
    //         return response_t(resp::server_error, error_desc(header));
    //     break;
    // default: break;
    // }

    // header does not recognized, return unrecognized
    return response_t(resp::unrecognized, header);
}

retrieve_command_t::response_t
retrieve_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    header_t hdr;
    std::copy(header.begin(), header.begin() + sizeof(header_t),
              reinterpret_cast<char *>(&hdr));

    hdr.prepare_deserialization();

    if (hdr.magic != header_t::response_magic)
        throw error_t(mc::err::internal_error, "Bad magic in response.");
    // The response should have uint32_t flags in extras.
    if (hdr.extras_len != sizeof(uint32_t))
        throw error_t(mc::err::internal_error, "Bad extras length.");

    if (!hdr.status)
        return retrieve_command_t::response_t(0, hdr.body_len,
                                              hdr.cas,
                                              retrieve_command_t::set_body);

    return retrieve_command_t::response_t(resp::not_found, hdr.body_len);
}

std::string retrieve_command_t::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    result.append(key);
    return result;
}

void retrieve_command_t::set_body(uint32_t &flags,
                                  std::string &body,
                                  const std::string &data) {
    std::copy(data.begin(), data.begin() + sizeof(flags),
              reinterpret_cast<char *>(&flags));
    flags = ntohl(flags);
    body = data.substr(sizeof(flags));
}

template<bool has_extras>
typename storage_command_t<has_extras>::response_t
storage_command_t<has_extras>::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    header_t hdr;
    std::copy(header.begin(), header.begin() + sizeof(header_t),
              reinterpret_cast<char *>(&hdr));

    hdr.prepare_deserialization();

    if (hdr.magic != header_t::response_magic)
        throw error_t(mc::err::internal_error, "Bad magic in response.");

    if (!hdr.status)
        return response_t(resp::stored);

    return response_t(resp::not_stored, "key (does not) exist");
}

template<>
std::string storage_command_t<true>::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = code;
    if (opts.cas) hdr.cas = opts.cas;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));


    uint32_t flags;
    flags = htonl(opts.flags);
    result += std::string (reinterpret_cast<char *>(&flags), sizeof(flags));
    // We use flags for exporation too.
    flags = htonl(static_cast<uint32_t>(opts.expiration));
    result += std::string (reinterpret_cast<char *>(&flags), sizeof(flags));

    result.append(key);
    result.append(data);
    return result;
}


template<>
std::string storage_command_t<false>::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = code;
    if (opts.cas) hdr.cas = opts.cas;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    result.append(key);
    result.append(data);
    return result;
}

template class storage_command_t<true>;
template class storage_command_t<false>;

incr_decr_command_t::response_t
incr_decr_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    header_t hdr;
    std::copy(header.begin(), header.begin() + sizeof(header_t),
              reinterpret_cast<char *>(&hdr));

    hdr.prepare_deserialization();

    if (hdr.magic != header_t::response_magic)
        throw error_t(mc::err::internal_error, "Bad magic in response.");
    if (hdr.body_len != sizeof(uint64_t))
        throw error_t(mc::err::internal_error, "Bad body length.");

    if (!hdr.status)
        return incr_decr_command_t::response_t(0, hdr.body_len,
                                              hdr.cas,
                                              incr_decr_command_t::set_body);

    return incr_decr_command_t::response_t(resp::not_found, hdr.body_len);
}

std::string incr_decr_command_t::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    //Extras
    uint64_t val = htobe64(value);
    result += std::string (reinterpret_cast<char *>(&val), sizeof(val));
    val = htobe64(opts.initial);
    result += std::string (reinterpret_cast<char *>(&val),
                           sizeof(val));
    uint32_t expire = htonl(static_cast<uint32_t>(opts.expiration));
    result += std::string (reinterpret_cast<char *>(&expire), sizeof(expire));

    result.append(key);
    return result;

}

void incr_decr_command_t::set_body(uint32_t &,
                                   std::string &body,
                                   const std::string &data) {
    uint64_t value;
    std::copy(data.begin(), data.end(), reinterpret_cast<char *>(&value));
    value = be64toh(value);
    body = boost::lexical_cast<std::string>(value);
}

delete_command_t::response_t
delete_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    header_t hdr;
    std::copy(header.begin(), header.begin() + sizeof(header_t),
              reinterpret_cast<char *>(&hdr));

    hdr.prepare_deserialization();

    if (hdr.magic != header_t::response_magic)
        throw error_t(mc::err::internal_error, "Bad magic in response.");

    if (!hdr.status)
        return response_t(resp::deleted);

    return response_t(resp::not_found, "key (does not) exist");
}

std::string delete_command_t::serialize() const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = api::delete_code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    result.append(key);
    return result;
}

} // namespace bin
} // namespace proto
} // namespace mc

