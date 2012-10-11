/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: commands parser.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-27 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_PROTO_PARSER_H
#define MCACHE_PROTO_PARSER_H

#include <boost/utility/enable_if.hpp>
#include <boost/mpl/has_xxx.hpp>

namespace mc {
namespace proto {
namespace aux {

/** Defines metafunction that recognize classes with multi_response tag.
 */
BOOST_MPL_HAS_XXX_TRAIT_DEF(multi_response_tag);

/** Defines metafunction that recognize classes with body tag.
 */
BOOST_MPL_HAS_XXX_TRAIT_DEF(body_tag);

} // namespace aux

/** This class provides interface for serializing and deserializing commands to
 * io object.
 */
template <typename connection_t>
class command_parser_t {
public:
    /** C'tor.
     */
    command_parser_t(connection_t &connection)
        : connection(&connection)
    {}

    /** Sends command to memcache server.
     */
    template <typename command_t>
    typename command_t::response_t send(const command_t &command) {
        // send serialized command to server
        connection->write(command.serialize());
        // deserialize server command response
        return deserialize_response(command);
    }

protected:
    /** Deserializes server response for multi response commands.
     */
    template <typename command_t>
    typename boost::enable_if<
        aux::has_multi_response_tag<command_t>,
        typename command_t::response_t
    >::type deserialize_response(const command_t &command) {
        // TODO(burlog): add support for multiget
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
        std::string header = connection->read(command.header_delimiter());
        response_t response = command.deserialize_header(header);

        // if response contains body fetch it and return response
        if (response) deserialize_body(response);
        return response;
    }

    /** Response expects body so retrieve and parse it.
     */
    template <typename response_t>
    typename boost::enable_if<aux::has_body_tag<response_t>, void>::type
    deserialize_body(response_t &response) {
        response.set_body(connection->read(response.expected_body_size()));
    }

    /** Does nothing.
     */
    template <typename response_t>
    typename boost::disable_if<aux::has_body_tag<response_t>, void>::type
    deserialize_body(response_t &) {}

    connection_t *connection; //!< wire to server
};

} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_PARSER_H */

