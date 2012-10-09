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
 *       2012-09-24 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_PROTO_BINARY_H
#define MCACHE_PROTO_BINARY_H

#include <arpa/inet.h>
#include <string>

#include <mcache/proto/opts.h>
#include <mcache/proto/response.h>

namespace mc {
namespace proto {
namespace bin {

/** The structure representing binary protocol header. */
struct header_t {
public:
    static const uint8_t request_magic = 0x80;
    static const uint8_t response_magic = 0x81;

    header_t() {}
    header_t(uint16_t key_len,
             uint32_t body_len = 0,
             uint8_t extras_len = 0):
        magic(request_magic),
        opcode(0x00),
        key_len(key_len),
        extras_len(extras_len),
        data_type(0x00),reserved(0x00),
        body_len(body_len),
        opaque(0x00), cas(0x00) {}

    uint8_t magic;
    uint8_t opcode;
    uint16_t key_len;
    uint8_t extras_len;
    uint8_t data_type;
    union {
        uint16_t reserved;
        uint16_t status;
    };
    uint32_t body_len;
    uint32_t opaque;
    uint64_t cas;

    /** Method for preparing serialization of header. It switches the
        endianity for every member. (hton)*/
    void prepare_serialization();

    /** Method for deserialization preprocessing. It switches the
        endianity for every member. (ntoh)*/
    void prepare_deserialization();
};


/** Base class of all commands.
 */
class command_t {
public:
    // majority of commands use single response
    typedef single_response_t response_t;

    /** Returns txt protocol header delimiter.
     */
    std::size_t header_delimiter() const { return 24;}

    /** Deserialize server error strings. If response is not general error
     * resp::unrecognized response is returned. Method should be overloaded by
     * paricular commands.
     */
    response_t deserialize_header(const std::string &header) const;

protected:
    command_t(uint16_t key_len, uint32_t body_len = 0, uint8_t extras_len = 0):
        hdr(key_len, body_len, extras_len) {}
    /** The header of command. */
    mutable header_t hdr;
};

/** Base class for all retrieve commands.
 */
class retrieve_command_t: public command_t {
public:
    // retrieval commands responses contains useless foother
    typedef single_retrival_response_t response_t;

    /** C'tor.
     */
    explicit retrieve_command_t(const std::string &key):
        command_t(static_cast <uint16_t>(key.length()),
                  static_cast <uint32_t>(key.length())),
        key(key) {}

    /** Returns txt protocol header delimiter.
     */
    std::size_t header_delimiter() const { return 24;}

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

    /** The method for setting the body of response. */
    static void set_body(uint32_t &flags,
                         std::string &body,
                         const std::string &data);
protected:
    /** Serialize retrieve command.
     */
    std::string serialize(uint8_t code) const;

    std::string key; //!< for which key data should be retrieved
};

/** Base class for all storage commands.
 */
template<bool has_extras = true>
class storage_command_t: public command_t {
public:
    /** C'tor.
     */
    storage_command_t(const std::string &key,
                      const std::string &data,
                      const opts_t &opts)
        :command_t(static_cast <uint16_t>(key.length()),
                   static_cast <uint32_t>(key.length() + data.length() +
                                          (has_extras ? extras_length : 0)),
                   static_cast <uint8_t>(has_extras ? extras_length : 0)),
        key(key), data(data), opts(opts)
    {}

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

protected:
    /** Serialize retrieve command.
     */
    std::string serialize(uint8_t code) const;

    std::string key;  //!< for which key data should be retrieved
    std::string data; //!< data to store
    opts_t opts;      //!< command options

private:
    /** The length of extras. */
    static const std::size_t extras_length = 8;
};

/** class for delete command
 */
class delete_command_t: public command_t {
public:
    /** C'tor.
     */
    explicit delete_command_t(const std::string &key):
        command_t(static_cast <uint16_t>(key.length()),
                  static_cast <uint32_t>(key.length())),
        key(key) {}

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

    /** Serialize retrieve command.
     */
    std::string serialize() const;

protected:
    std::string key; //!< for which key data should be retrieved
};

/** Injects name to particular command class.
 */
template <typename parent_t, uint8_t code>
class op_code_injector: public parent_t {
public:
    /** C'tor.
     */
    template <typename type0_t>
    op_code_injector(const type0_t &val0): parent_t(val0) {}

    /** C'tor.
     */
    template <typename type0_t, typename type1_t>
    op_code_injector(const type0_t &val0, const type1_t &val1)
        : parent_t(val0, val1)
    {}

    /** C'tor.
     */
    template <typename type0_t, typename type1_t, typename type2_t>
    op_code_injector(const type0_t &val0, const type1_t &val1, const type2_t &val2)
        : parent_t(val0, val1, val2)
    {}

    /** Injects code of command to parent serialize method.
     */
    std::string serialize() const { return parent_t::serialize(code);}
};

/** Used as namespace with class behaviour for protocol api.
 */
class api {
public:
    // commands name
static const uint8_t get_code = 0x00;
static const uint8_t gets_code = 0x00;
static const uint8_t set_code = 0x01;
static const uint8_t add_code = 0x02;
static const uint8_t replace_code = 0x03;
static const uint8_t delete_code = 0x04;
static const uint8_t increment_code = 0x05;
static const uint8_t decrement_code = 0x06;
static const uint8_t quit_code = 0x07;
static const uint8_t flush_code = 0x08;
static const uint8_t getq_code = 0x09;
static const uint8_t noop_code = 0x0A;
static const uint8_t version_code = 0x0B;
static const uint8_t getk_code = 0x0C;
static const uint8_t getkq_code = 0x0D;
static const uint8_t append_code = 0x0E;
static const uint8_t prepend_code = 0x0F;
static const uint8_t stat_code = 0x10;
static const uint8_t setq_code = 0x11;
static const uint8_t addq_code = 0x12;
static const uint8_t replaceq_code = 0x13;
static const uint8_t deleteq_code = 0x14;
static const uint8_t incrementq_code = 0x15;
static const uint8_t decrementq_code = 0x16;
static const uint8_t quitq_code = 0x17;
static const uint8_t flushq_code = 0x18;
static const uint8_t appendq_code = 0x19;
static const uint8_t prependq_code = 0x1A;

    // protocol api table
    typedef op_code_injector<retrieve_command_t, get_code> get_t;
    typedef op_code_injector<retrieve_command_t, gets_code> gets_t;
    typedef op_code_injector<storage_command_t<true>, set_code> set_t;
    typedef op_code_injector<storage_command_t<true>, add_code> add_t;
    typedef op_code_injector<storage_command_t<true>, replace_code> replace_t;
    typedef op_code_injector<storage_command_t<false>, append_code> append_t;
    typedef op_code_injector<storage_command_t<false>, prepend_code> prepend_t;
    // typedef op_code_injector<storage_command_t, cas_code> cas_t;


    //typedef op_code_injector<inc_command_t, inc_code> inc_t;
    //typedef op_code_injector<dec_command_t, dec_code> dec_t;
    typedef delete_command_t delete_t;
    //typedef touch_command_t touch_t;
};

} // namespace bin
} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_BINARY_H */

