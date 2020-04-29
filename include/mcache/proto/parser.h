/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: commands parser.
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
 *       2012-09-27 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_PROTO_PARSER_H
#define MCACHE_PROTO_PARSER_H

#include <string>

namespace mc {
namespace proto {
namespace aux {

template <typename, typename = bool>
struct has_set_body {static constexpr bool value = false;};

template <typename response_t>
struct has_set_body<
    response_t,
    decltype(
        std::declval<response_t>().set_body(std::declval<std::string>()),
        true
    )
> {static constexpr bool value = true;};

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
    /** Deserializes server response for single response commands.
     */
    template <typename command_t>
    typename command_t::response_t
    deserialize_response(const command_t &command) {
        // fetch response header (txt: first line of response)
        typedef typename command_t::response_t response_t;
        std::string header = connection->read(command.header_delimiter());
        response_t response = command.deserialize_header(header);

        // if response contains body then fetch it and return response
        deserialize_body(response);
        return response;
    }

    /** Response expects body so retrieve and parse it.
     */
    template <typename response_t>
    std::enable_if_t<aux::has_set_body<response_t>::value>
    deserialize_body(response_t &response) {
        std::size_t body_size = response.expected_body_size();
        if (body_size) response.set_body(connection->read(body_size));
    }

    /** Does nothing.
     */
    template <typename response_t>
    std::enable_if_t<!aux::has_set_body<response_t>::value>
    deserialize_body(response_t &) {}

    connection_t *connection; //!< wire to server
};

} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_PARSER_H */

