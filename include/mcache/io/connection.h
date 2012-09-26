/*
 * FILE             $Id: $
 *
 * DESCRIPTION      I/O object for communication with memcache server.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-19 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_IO_CONNECTIONS_H
#define MCACHE_IO_CONNECTIONS_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/has_xxx.hpp>

#include <mcache/error.h>

namespace mc {
namespace io {

/** Socket options.
 */
class opts_t {
public:
    // TODO(burlog): add socket options
};

namespace tcp {
namespace aux {

/** Defines metafunction that recognize classes with multi_response tag.
 */
BOOST_MPL_HAS_XXX_TRAIT_DEF(multi_response_tag);

/** Defines metafunction that recognize classes with body tag.
 */
BOOST_MPL_HAS_XXX_TRAIT_DEF(body_tag);

} // namespace aux

/** I/O object that holds one tcp socket to memcache server and provides
 * interface for sending messages to them.
 */
class connection_t {
public:
    /** C'tor.
     */
    connection_t(const std::string &addr, opts_t opts);

    /** Sends command to memcache server.
     */
    template <typename command_t>
    typename command_t::response_t send(const command_t &command) {
        // send serialized command to server
        write(command.serialize());
        // deserialize server command response
        return deserialize_response(command);
    }

protected:
    /** Sends all data to server.
     */
    void write(const std::string &data);

    /** Reads data till delimiter and returns it.
     */
    std::string read(const std::string &delimiter);

    /** Reads bytes count data and returns it.
     */
    std::string read(std::size_t bytes);

    /** Deserializes server response for multi response commands.
     */
    template <typename command_t>
    typename boost::enable_if<
        aux::has_multi_response_tag<command_t>,
        typename command_t::response_t
    >::type deserialize_response(const command_t &command) {
        // TODO(burlog): add support for multiget (here should be cycle)
    }

    /** Deserializes server response for single response commands.
     */
    template <typename command_t>
    typename boost::disable_if<
        aux::has_multi_response_tag<command_t>,
        typename command_t::response_t
    >::type deserialize_response(const command_t &command) {
        // fetch response header (txt: first line of response)
        typedef typename command_t::response_t response_t;
        std::string header = read(command.header_delimiter());
        response_t response = command.deserialize_header(header);

        // if response contains body fetch it and return response
        deserialize_body(response);
        return response;
    }

    /** Response expects body so retrieve and parse it.
     */
    template <typename response_t>
    typename boost::enable_if<aux::has_body_tag<response_t>, void>::type
    deserialize_body(response_t &response) {
        response.set_body(read(response.expected_body_size()));
    }

    /** Does nothing.
     */
    template <typename response_t>
    typename boost::disable_if<aux::has_body_tag<response_t>, void>::type
    deserialize_body(response_t &) {}

    /** Pimple class.
     */
    class pimple_connection_t;

    // shortcut
    typedef boost::shared_ptr<pimple_connection_t> pimple_connection_ptr_t;

    pimple_connection_ptr_t socket; //!< hides i/o implementation
};

} // namespace tcp

namespace udp {

// TODO(burlog): add support for udp protocol

} // namespace udp

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_CONNECTIONS_H */

