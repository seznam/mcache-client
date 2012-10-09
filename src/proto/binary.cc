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

#include "error.h"
#include "mcache/proto/binary.h"

namespace mc {
namespace proto {
namespace bin {

// TODO (Lubos) Pokud by se melo pouzivat na solarisu, tak je potreba
// tyhle funkce vytahnout z glibc (htobe atp.
void header_t::prepare_serialization() {
    key_len = htobe16(key_len);
    reserved = htobe16(reserved);
    body_len = htobe32(body_len);
    opaque = htobe32(opaque);
    cas = htobe64(cas);
}

void header_t::prepare_deserialization() {
    key_len = be16toh(key_len);
    reserved = be16toh(reserved);
    body_len = be32toh(body_len);
    opaque = be32toh(opaque);
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

#warning tady pridat vyhozeni chyby, az asi pridam tu chybu.
    // if (hdrp->magic != header_t::response_magic) throw error_t
    // if (hdrp->extras_len != 4) throw error_t

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
    flags = be32toh(flags);
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

#warning tady pridat vyhozeni chyby, az asi pridam tu chybu.
    // if (hdrp->magic != header_t::response_magic) throw error_t

    if (!hdr.status)
        return response_t(resp::stored);

#warning poladit chyby
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
    flags = htobe32(opts.flags);
    result += std::string (reinterpret_cast<char *>(&flags), sizeof(flags));
    // We use flags for exporation too.
    flags = htobe32(opts.expiration);
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

// incr_decr_command_t::response_t
// incr_decr_command_t::deserialize_header(const std::string &header) const {
//     // reject empty response
//     if (header.empty()) return response_t(resp::empty, "empty response");

//     // try parse retrieve responses
//     switch (header[0]) {
//     case 'T':
//         // fail response
//         if (boost::starts_with(header, "TOUCHED"))
//             return response_t(resp::touched);
//         break;
//     case 'N':
//         if (boost::starts_with(header, "NOT_FOUND"))
//             return response_t(resp::not_found, "key does not exist");
//         break;
//     case '0'...'9': {
//         // <value>\r\n - ok response
//         std::istringstream is(header);
//         uint64_t value;
//         if (is >> value)
//             return response_t(resp::ok, boost::trim_copy(header));
//         break;
//         }
//     default: break;
//     }

//     // header does not recognized, try global errors
//     return command_t::deserialize_header(header);

// }

// std::string incr_decr_command_t::serialize(const char *name) const {
//     // <command name> <key> <value> [noreply]\r\n
//     // TODO(burlog): check whether key fulfil protocol constraints
//     std::ostringstream os;
//     os << name << ' ' << key << ' ' << value << header_delimiter();
//     return os.str();
//     // TODO(burlog): check whether key fulfil protocol constraints
//     hdr.opcode = api::delete_code;
//     hdr.prepare_serialization();
//     std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
//     result.append(key);
//     return result;
// }

delete_command_t::response_t
delete_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    header_t hdr;
    std::copy(header.begin(), header.begin() + sizeof(header_t),
              reinterpret_cast<char *>(&hdr));

    hdr.prepare_deserialization();

#warning tady pridat vyhozeni chyby, az asi pridam tu chybu.
    // if (hdrp->magic != header_t::response_magic) throw error_t

    if (!hdr.status)
        return response_t(resp::deleted);

#warning poladit chyby
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

