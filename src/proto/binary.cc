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
#include <arpa/inet.h>
#include <sstream>
#include <map>
#include <boost/lexical_cast.hpp>

#include "error.h"
#include "mcache/proto/binary.h"

namespace mc {
namespace proto {
namespace bin {
namespace {

enum status_code_t {
    ok = 0x0000,                // No error
    not_found = 0x0001,         // Key not found
    exists = 0x0002,            // Key exists
    value_too_large = 0x0003,   // Value too large
    invalid_arguments = 0x0004, // Invalid arguments
    not_stored = 0x0005,        // Item not stored
    non_numerci_value = 0x0006, // Incr/Decr on non-numeric value.
    unknown_command = 0x0081,   // Unknown command
    out_of_memory = 0x0082,     // Out of memory
};

std::map<int, int> code_mapping;

void initialize_map() {
    code_mapping[0x0000] = mc::proto::resp::ok;
    code_mapping[0x0001] = mc::proto::resp::not_found;
    code_mapping[0x0002] = mc::proto::resp::exists;
    code_mapping[0x0003] = mc::proto::resp::server_error;
    code_mapping[0x0004] = mc::proto::resp::client_error;
    code_mapping[0x0005] = mc::proto::resp::not_stored;
    code_mapping[0x0006] = mc::proto::resp::client_error;
    code_mapping[0x0081] = mc::proto::resp::error;
    code_mapping[0x0082] = mc::proto::resp::server_error;
}

int translate_status_to_response(int code) {
    const std::map<int, int>::const_iterator it = code_mapping.find(code);
    if (it == code_mapping.end()) return mc::proto::resp::error;
    return it->second;
}

const header_t *as_header(const std::string &data) {
    return reinterpret_cast<const header_t *>(&data[0]);
}

std::ostream &operator<<(std::ostream &os, const header_t &h) {
    os << "magic: " << (int)h.magic << std::endl;
    os << "opcode: " << (int)h.opcode << std::endl;
    os << "key_len: " << (int)h.key_len << std::endl;
    os << "extras_len: " << (int)h.extras_len << std::endl;
    os << "data_type: " << (int)h.data_type << std::endl;
    os << "status: " << (int)h.status << std::endl;
    os << "body_len: " << (int)h.body_len << std::endl;
    os << "opaque: " << (int)h.opaque << std::endl;
    os << "cas: " << (int)h.cas << std::endl;
    return os;
}

} // namespace

/** Init function of the bin protocol module.
 */
void init() {
    initialize_map();
}

// TODO(Lubos): Pokud by se melo pouzivat na solarisu, tak je potreba
// tyhle funkce vytahnout z glibc htobe atp.

header_t::header_t(const std::string &data)
    : magic(as_header(data)->magic),
      opcode(as_header(data)->opcode),
      key_len(ntohs(as_header(data)->key_len)),
      extras_len(as_header(data)->extras_len),
      data_type(as_header(data)->data_type),
      reserved(ntohs(as_header(data)->reserved)),
      body_len(ntohl(as_header(data)->body_len)),
      opaque(ntohl(as_header(data)->opaque)),
      cas(be64toh(as_header(data)->cas))
{}

void header_t::prepare_serialization() {
    key_len = htons(key_len);
    reserved = htons(reserved);
    body_len = htonl(body_len);
    opaque = htonl(opaque);
    cas = htobe64(cas);
}

retrieve_command_t::response_t
retrieve_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::syntax, "bad magic in response");

    // check response status
    if (!hdr.status) {
        // the response should have uint32_t flags in extras.
        if (hdr.extras_len != sizeof(uint32_t))
            return response_t(resp::invalid, "bad extras length");
        return response_t(0, hdr.body_len, hdr.cas, set_body);
    }
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
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
                                  const std::string &data)
{
    std::copy(data.begin(), data.begin() + sizeof(flags),
              reinterpret_cast<char *>(&flags));
    flags = ntohl(flags);
    body = data.substr(sizeof(flags));
}

template <bool has_extras>
typename storage_command_t<has_extras>::response_t
storage_command_t<has_extras>
::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::syntax, "bad magic in response");

    // check response status
    if (!hdr.status) return response_t(resp::stored);
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

template <>
std::string storage_command_t<true>::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    // prepare request
    hdr.opcode = code;
    if (opts.cas) hdr.cas = opts.cas;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    // append flags
    uint32_t flags;
    flags = htonl(opts.flags);
    result += std::string(reinterpret_cast<char *>(&flags), sizeof(flags));

    // append expiration; we use flags for expiration too
    flags = htonl(static_cast<uint32_t>(opts.expiration));
    result += std::string(reinterpret_cast<char *>(&flags), sizeof(flags));

    // accomplish request
    result.append(key);
    result.append(data);
    return result;
}

template <>
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

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::syntax, "bad magic in response");

    // check status
    if (!hdr.status) {
        if (hdr.body_len != sizeof(uint64_t))
            return response_t(resp::invalid, "bad body length");
        return response_t(0, hdr.body_len, hdr.cas, set_body);
    }
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string incr_decr_command_t::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    // prepare request
    hdr.opcode = code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    // append extras
    uint64_t val = htobe64(value);
    result += std::string(reinterpret_cast<char *>(&val), sizeof(val));
    val = htobe64(opts.initial);
    result += std::string(reinterpret_cast<char *>(&val), sizeof(val));
    uint32_t expire = htonl(static_cast<uint32_t>(opts.expiration));
    result += std::string(reinterpret_cast<char *>(&expire), sizeof(expire));

    // accomplish request
    result.append(key);
    return result;

}

void incr_decr_command_t::set_body(uint32_t &,
                                   std::string &body,
                                   const std::string &data)
{
    uint64_t value;
    std::copy(data.begin(), data.end(), reinterpret_cast<char *>(&value));
    value = be64toh(value);
    body = boost::lexical_cast<std::string>(value);
}

delete_command_t::response_t
delete_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::syntax, "bad magic in response");

    // check status
    if (!hdr.status) return response_t(resp::deleted);
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string delete_command_t::serialize() const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = api::delete_code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    result.append(key);
    return result;
}

single_retrival_response_t::set_body_callback_t
touch_command_t::set_body = incr_decr_command_t::set_body;

touch_command_t::response_t
touch_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::syntax, "bad magic in response");

    // check status
    if (!hdr.status)
        return response_t(resp::touched, 0, hdr.body_len, hdr.cas, set_body);
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string touch_command_t::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    // prepare request
    hdr.opcode = code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));

    // extras
    uint32_t expire = htonl(static_cast<uint32_t>(expiration));
    result += std::string(reinterpret_cast<char *>(&expire), sizeof(expire));

    // accomplish request
    result.append(key);
    return result;

}

} // namespace bin
} // namespace proto
} // namespace mc

