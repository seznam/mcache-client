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
#include <boost/bind.hpp>
#include <arpa/inet.h>
#include <sstream>
#include <map>

#include "error.h"
#include "proto/aux.h"
#include "mcache/proto/binary.h"

// for protocol details:
// @see https://github.com/memcached/memcached/blob/master/doc/protocol-binary.xml

namespace mc {
namespace proto {
namespace bin {
namespace {

// TODO(Lubos): On solaris we must define htobe, ntohs, ...

// forward
class header_t;

/** Cast string to pointer to header.
 */
static const header_t *as_header(const std::string &data) {
    return reinterpret_cast<const header_t *>(&data[0]);
}

/** The structure representing binary protocol header.
 */
struct header_t {
public:
    // magic constants
    static const uint8_t request_magic = 0x80;
    static const uint8_t response_magic = 0x81;

    /** C'tor.
     */
    header_t(const std::string &data)
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

    /** C'tor.
     */
    header_t(uint16_t key_len, uint32_t body_len = 0, uint8_t extras_len = 0)
        : magic(request_magic),
          opcode(0x00),
          key_len(key_len),
          extras_len(extras_len),
          data_type(0x00),
          reserved(0x00),
          body_len(body_len),
          opaque(0x00),
          cas(0x00)
    {}

    /** Method for preparing serialization of header. It switches the
     * endianity for every member. (hton)
     */
    void prepare_serialization() {
        key_len = htons(key_len);
        reserved = htons(reserved);
        body_len = htonl(body_len);
        opaque = htonl(opaque);
        cas = htobe64(cas);
    }

    uint8_t magic;         //!< protocol magic
    uint8_t opcode;        //!< command code
    uint16_t key_len;      //!< the length of the key
    uint8_t extras_len;    //!< the length of extra info in packet
    uint8_t data_type;     //!< reserved for future usage
    union {
        uint16_t reserved; //!< response for future usage
        uint16_t status;   //!< status of the response
    };
    uint32_t body_len;     //!< length in bytes of extra + key + value
    uint32_t opaque;       //!< will be copied back to you in the response
    uint64_t cas;          //!< unique identifier of data(data version)
};

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

std::map<int, resp::response_code_t> code_mapping;

static void initialize_map() {
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

/** Translate binary status codes into txt response codes.
 */
static resp::response_code_t translate_status_to_response(int code) {
    const std::map<int, resp::response_code_t>::const_iterator
        it = code_mapping.find(code);
    if (it == code_mapping.end()) return mc::proto::resp::error;
    return it->second;
}

} // namespace

/** Init function of the bin protocol module.
 */
void init() {
    initialize_map();
}

retrieve_command_t::response_t
retrieve_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::unrecognized, "bad magic in response");

    // check response status
    if (!hdr.status) {
        // the response should have uint32_t flags in extras.
        if (hdr.extras_len != sizeof(uint32_t))
            return response_t(resp::invalid, "bad extras length");
        return response_t(0, hdr.body_len, hdr.cas,
                          boost::bind(set_body, _1, _2, _3, hdr.key_len));
    }
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string retrieve_command_t::serialize(uint8_t code) const {
    aux::check_key(key);
    header_t hdr(key_len, body_len, extras_len);
    hdr.opcode = code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    result.append(key);
    return result;
}

void retrieve_command_t::set_body(uint32_t &flags,
                                  std::string &body,
                                  const std::string &data,
                                  uint16_t key_len)
{
    std::copy(data.begin(), data.begin() + sizeof(flags),
              reinterpret_cast<char *>(&flags));
    flags = ntohl(flags);
    body = data.substr(sizeof(flags) + key_len);
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
        return response_t(resp::unrecognized, "bad magic in response");

    // check response status
    if (!hdr.status) return response_t(resp::stored);
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

template <>
std::string storage_command_t<true>::serialize(uint8_t code) const {
    // prepare request
    aux::check_key(key);
    header_t hdr(key_len, body_len, extras_len);
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
    aux::check_key(key);
    header_t hdr(key_len, body_len, extras_len);
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
        return response_t(resp::unrecognized, "bad magic in response");

    // check status
    if (!hdr.status) {
        if (hdr.body_len != sizeof(uint64_t))
            return response_t(resp::invalid, "bad body length");
        return response_t(resp::ok, hdr.body_len, set_body);
    }
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string incr_decr_command_t::serialize(uint8_t code) const {
    // prepare request
    aux::check_key(key);
    header_t hdr(key_len, body_len, extras_len);
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

void incr_decr_command_t::set_body(std::string &body, const std::string &data) {
    uint64_t value = 0;
    std::copy(data.begin(), data.end(), reinterpret_cast<char *>(&value));
    value = be64toh(value);
    std::ostringstream os;
    os << value;
    body = os.str();
}

delete_command_t::response_t
delete_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::unrecognized, "bad magic in response");

    // check status
    if (!hdr.status) return response_t(resp::deleted);
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string delete_command_t::serialize() const {
    aux::check_key(key);
    header_t hdr(key_len, body_len, extras_len);
    hdr.opcode = api::delete_code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    result.append(key);
    return result;
}

touch_command_t::response_t::set_body_callback_t
touch_command_t::set_body = incr_decr_command_t::set_body;

touch_command_t::response_t
touch_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // parse header && check protocol magic
    header_t hdr(header);
    if (hdr.magic != header_t::response_magic)
        return response_t(resp::unrecognized, "bad magic in response");

    // check status
    if (!hdr.status)
        return response_t(resp::touched, hdr.body_len, set_body);
    return response_t(translate_status_to_response(hdr.status), hdr.body_len);
}

std::string touch_command_t::serialize(uint8_t code) const {
    // prepare request
    aux::check_key(key);
    header_t hdr(key_len, body_len, extras_len);
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

