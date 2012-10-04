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

test_t::response_t
test_t::deserialize_header(const std::string &header) const {
    std::size_t extra_len = header[4];
    std::size_t body_len = ::ntohl(reinterpret_cast<const uint32_t *>
            (header.data())[2]);
    LOG(INFO3, "LEN.... %zd %zd", extra_len , body_len);
    return response_t(0, body_len - extra_len, 0, 0);
}

std::string test_t::serialize() const {
    header_t hdr;
    hdr.magic = 0x80;
    hdr.opcode = 0x00;
    hdr.key_len = 5;
    hdr.extras_len = 0;
    hdr.data_type = 0;
    hdr.reserved = 0;
    hdr.body_len = 5;
    hdr.opaque = 0;
    hdr.cas = 0;
    hdr.prepare_serialization();

    std::string hdrs(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    std::ostringstream os;
    os
       // << char(0x80) << char(0x00) << char(0x00) << char(0x05)
       // << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       // << char(0x00) << char(0x00) << char(0x00) << char(0x05)
       // << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       // << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       // << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       << char(0x48) << char(0x65) << char(0x6c) << char(0x6c)
       << char(0x6f);
    return hdrs +
        os.str();
}





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
    uint32_t flags;
    std::copy(header.begin() + sizeof(header_t), header.end(),
              reinterpret_cast<char *>(&flags));
    flags = be32toh(flags);
    hdr.prepare_deserialization();

    // try parse retrieve responses
    // switch (header[0]) {
    // case 'E':
    //     if (boost::starts_with(header, "END"))
    //         return response_t(resp::not_found, "not found");
    //     break;
    // case 'V':
    //     if (boost::starts_with(header, "VALUE"))
    //         return deserialize_value_resp(header);
    //     break;
    // default: break;
    // }

    // header does not recognized, try global errors

#warning tady pridat vyhozeni chyby, az asi pridam tu chybu.
    // if (hdrp->magic != header_t::response_magic) throw error_t
    // if (hdrp->extras_len != 4) throw error_t

    if (!hdr.status)
        return retrieve_command_t::response_t
            (flags,
             hdr.body_len - hdr.extras_len,
             hdr.cas, footer_size);

    return response_t(command_t::deserialize_header(header));
}

std::string retrieve_command_t::serialize(uint8_t code) const {
    // TODO(burlog): check whether key fulfil protocol constraints
    hdr.opcode = code;
    hdr.prepare_serialization();
    std::string result(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    result.append(key);
    return result;
}

storage_command_t::response_t
storage_command_t::deserialize_header(const std::string &header) const {
    // reject empty response
    if (header.empty()) return response_t(resp::empty, "empty response");

    // // try parse retrieve responses
    // switch (header[0]) {
    // case 'E':
    //     if (boost::starts_with(header, "EXISTS"))
    //         return response_t(resp::exists, "cas id expired");
    //     break;
    // case 'N':
    //     if (boost::starts_with(header, "NOT_FOUND"))
    //         return response_t(resp::not_found, "cas id is invalid");
    //     if (boost::starts_with(header, "NOT_STORED"))
    //         return response_t(resp::not_stored, "key (does not) exist");
    //     break;
    // case 'S':
    //     if (boost::starts_with(header, "STORED"))
    //         return response_t(resp::stored);
    //     break;
    // default: break;
    // }

    // header does not recognized, try global errors
    return command_t::deserialize_header(header);
}

std::string storage_command_t::serialize(uint8_t code) const {
    // <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n
    // cas <key> <flags> <exptime> <bytes> <cas unique> [noreply]\r\n
    // TODO(burlog): check whether key fulfil protocol constraints
    // std::ostringstream os;
    // os << name << ' '
    //    << key << ' '
    //    << opts.flags << ' '
    //    << opts.expiration << ' '
    //    << data.size();
    // if (opts.cas) os << ' ' << opts.cas;
    // os << header_delimiter() << data << header_delimiter();
    // return os.str();
    return "";
}

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
// }

// delete_command_t::response_t
// delete_command_t::deserialize_header(const std::string &header) const {
//     // reject empty response
//     if (header.empty()) return response_t(resp::empty, "empty response");

//     // try parse retrieve responses
//     switch (header[0]) {
//     case 'D':
//         if (boost::starts_with(header, "DELETED"))
//             return response_t(resp::deleted);
//         break;
//     case 'N':
//         if (boost::starts_with(header, "NOT_FOUND"))
//             return response_t(resp::not_found, "key does not exist");
//         break;
//     default: break;
//     }

//     // header does not recognized, try global errors
//     return response_t(command_t::deserialize_header(header));

// }

// std::string delete_command_t::serialize() const {
//     std::string result;
//     result.append("delete ").append(key).append(header_delimiter());
//     return result;
// }

// command Operation codes
// const uint8_t api::get_code = 0x00;
// const uint8_t api::gets_code = 0x00;
// const uint8_t api::set_code = 0x01;
// const uint8_t api::add_code = 0x02;
// const uint8_t api::replace_code = 0x03;
// const uint8_t api::delete_code = 0x04;
// const uint8_t api::increment_code = 0x05;
// const uint8_t api::decrement_code = 0x06;
// const uint8_t api::quit_code = 0x07;
// const uint8_t api::flush_code = 0x08;
// const uint8_t api::getq_code = 0x09;
// const uint8_t api::noop_code = 0x0A;
// const uint8_t api::version_code = 0x0B;
// const uint8_t api::getk_code = 0x0C;
// const uint8_t api::getkq_code = 0x0D;
// const uint8_t api::append_code = 0x0E;
// const uint8_t api::prepend_code = 0x0F;
// const uint8_t api::stat_code = 0x10;
// const uint8_t api::setq_code = 0x11;
// const uint8_t api::addq_code = 0x12;
// const uint8_t api::replaceq_code = 0x13;
// const uint8_t api::deleteq_code = 0x14;
// const uint8_t api::incrementq_code = 0x15;
// const uint8_t api::decrementq_code = 0x16;
// const uint8_t api::quitq_code = 0x17;
// const uint8_t api::flushq_code = 0x18;
// const uint8_t api::appendq_code = 0x19;
// const uint8_t api::prependq_code = 0x1A;

} // namespace bin
} // namespace proto
} // namespace mc

