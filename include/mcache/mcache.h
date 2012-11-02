/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Default instantiation of client class.
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

#ifndef MCACHE_MCACHE_H
#define MCACHE_MCACHE_H

#include <mcache/init.h>
#include <mcache/hash.h>
#include <mcache/client.h>
#include <mcache/proto/txt.h>
#include <mcache/proto/binary.h>
#include <mcache/server-proxy.h>
#include <mcache/server-proxies.h>
#include <mcache/pool/consistent-hashing.h>
#include <mcache/io/connections.h>
#include <mcache/io/connection.h>

namespace mc {
namespace thread {

// configuration classes
typedef mc::consistent_hashing_pool_config_t pool_config_t;
typedef mc::server_proxy_config_t server_proxy_config_t;

// defines types for client template
using proto::bin::api;
typedef mc::consistent_hashing_pool_t<murmur3_t> pool_t;
typedef io::caching_connection_pool_t<io::tcp::connection_t> connections_t;
typedef mc::server_proxy_t<lock_t, connections_t> server_proxy_t;
typedef mc::server_proxies_t<shared_array_t, server_proxy_t> server_proxies_t;

/// Defines default instantiation of the client template for thread enviroment.
typedef mc::client_template_t<pool_t, server_proxies_t, api> client_t;

} // namespace thread

namespace ipc {

// configuration classes
typedef mc::consistent_hashing_pool_config_t pool_config_t;
typedef mc::server_proxy_config_t server_proxy_config_t;

// defines types for client template
using proto::bin::api;
typedef mc::consistent_hashing_pool_t<murmur3_t> pool_t;
typedef io::single_connection_pool_t<io::tcp::connection_t> connections_t;
typedef mc::server_proxy_t<lock_t, connections_t> server_proxy_t;
typedef mc::server_proxies_t<shared_array_t, server_proxy_t> server_proxies_t;

/// Defines default instantiation of the client template for process enviroment.
typedef mc::client_template_t<pool_t, server_proxies_t, api> client_t;

} // namespace ipc
} // namespace mc

#endif /* MCACHE_MCACHE_H */

