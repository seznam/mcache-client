/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: text protocol
 *                  implementation base class.
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
 *       2012-09-24 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_PROTO_TXT_H
#define MCACHE_PROTO_TXT_H

#include <string>

#include <mcache/error.h>
#include <mcache/proto/opts.h>
#include <mcache/proto/response.h>

namespace mc {
namespace proto {
namespace txt {

/** Base class of all commands.
 */
class command_t {
public:
    // majority of commands use single response
    typedef single_response_t response_t;

    /** Returns txt protocol header delimiter.
     */
    const char *header_delimiter() const { return "\r\n";}

    /** Deserialize server error strings. If response is not general error
     * resp::unrecognized response is returned. Method should be overloaded by
     * paricular commands.
     */
    response_t deserialize_header(const std::string &header) const;
};

/** Base class for all retrieve commands.
 */
class retrieve_command_t: public command_t {
public:
    // retrieval commands responses contains useless foother
    typedef single_retrival_response_t response_t;

    /** C'tor.
     */
    explicit retrieve_command_t(const std::string &key): key(key) {}

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

    /** The method for setting the body of response. */
    static void set_body(uint32_t &flags,
                         std::string &body,
                         const std::string &data);

    const std::string key; //!< for which key data should be retrieved
    static const std::size_t footer_size = 7; //!< sizeof("\r\nEND\r\n")

protected:
    /** Serialize retrieve command.
     */
    std::string serialize(const char *name) const;
};

/** Base class for all storage commands.
 */
class storage_command_t: public command_t {
public:
    /** C'tor.
     */
    storage_command_t(const std::string &key,
                      const std::string &data,
                      const opts_t &opts = opts_t())
        : key(key), data(data), opts(opts)
    {}

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

    const std::string key; //!< for which key data should be retrieved

protected:
    /** Serialize retrieve command.
     */
    std::string serialize(const char *name) const;

    const std::string data; //!< data to store
    const opts_t opts;      //!< command options
};

/** Base class for incr and decr commands.
 */
class incr_decr_command_t: public command_t {
public:
    /** C'tor.
     */
    incr_decr_command_t(const std::string &key, uint64_t value,
                        const opts_t &opts = opts_t())
        : key(key), value(value)
    {
        if (opts.initial)
            throw mc::error_t(err::bad_argument, "initial not allowed for txt");
    }

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

    const std::string key; //!< for which key data should be retrieved

protected:
    /** Serialize retrieve command.
     */
    std::string serialize(const char *name) const;

    uint64_t value;  //!< amount by which the client wants to increase/decrease
};

/** Base class for incr and decr commands.
 */
class delete_command_t: public command_t {
public:
    /** C'tor.
     */
    explicit delete_command_t(const std::string &key): key(key) {}

    /** Deserialize responses for get and gets retrieve commands.
     */
    response_t deserialize_header(const std::string &header) const;

    /** Serialize retrieve command.
     */
    std::string serialize() const;

    const std::string key; //!< for which key data should be retrieved
};

/** Injects name to particular command class.
 */
template <typename parent_t, const char **name>
class name_injector: public parent_t {
public:
    /** C'tor.
     */
    template <typename type0_t>
    name_injector(const type0_t &val0): parent_t(val0) {}

    /** C'tor.
     */
    template <typename type0_t, typename type1_t>
    name_injector(const type0_t &val0, const type1_t &val1)
        : parent_t(val0, val1)
    {}

    /** C'tor.
     */
    template <typename type0_t, typename type1_t, typename type2_t>
    name_injector(const type0_t &val0, const type1_t &val1, const type2_t &val2)
        : parent_t(val0, val1, val2)
    {}

    /** Injects name of command to parent serialize method.
     */
    std::string serialize() const { return parent_t::serialize(*name);}
};

/** Used as namespace with class behaviour for protocol api.
 */
class api {
private:
    // commands name
    static const char *get_name;
    static const char *gets_name;
    static const char *set_name;
    static const char *add_name;
    static const char *replace_name;
    static const char *append_name;
    static const char *prepend_name;
    static const char *cas_name;
    static const char *incr_name;
    static const char *decr_name;
    static const char *touch_name;

public:
    // protocol api table
    typedef name_injector<retrieve_command_t, &get_name> get_t;
    typedef name_injector<retrieve_command_t, &gets_name> gets_t;
    typedef name_injector<storage_command_t, &set_name> set_t;
    typedef name_injector<storage_command_t, &add_name> add_t;
    typedef name_injector<storage_command_t, &replace_name> replace_t;
    typedef name_injector<storage_command_t, &append_name> append_t;
    typedef name_injector<storage_command_t, &prepend_name> prepend_t;
    typedef name_injector<storage_command_t, &cas_name> cas_t;
    typedef name_injector<incr_decr_command_t, &incr_name> incr_t;
    typedef name_injector<incr_decr_command_t, &decr_name> decr_t;
    typedef name_injector<incr_decr_command_t, &touch_name> touch_t;
    typedef delete_command_t delete_t;
};

} // namespace txt
} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_TXT_H */

