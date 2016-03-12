# Memcache client

C++ implementation of memcache client that support this features:

 * txt and binary application protocol,
 * TCP and UDP transport protocol,
 * consistent hashing pool of servers,
 * thread/process dead servers caching,
 * automatic dead servers restoration,
 * thread/process socket caching,
 * extensively customizable,
 * includes optional zlib compression,
 * python module.

## Connect the servers and do some simple operation

You can compose your client class as you wish. Look at the mc::client_t template
in the mcache/client.h. You can choose connection class and server pool classes
that match your needs. But if you didn't want to care about it then you can use
default composition that you can find in the mcache/mcache.h header file. Next
example show the usage:

```c++
#include <vector>
#include <string>
#include <iostream>

#include <mcache/mcache.h>

int main(int, char *[]) {
    // library initialization
    mc::init();

    try {
        // configuration
        mc::server_proxy_config_t scfg;
        scfg.io_opts.timeouts.connect = 3000;
        scfg.io_opts.timeouts.write = 3000;
        scfg.io_opts.timeouts.read = 3000;

        // connect the servers pool
        std::vector<std::string>
            servers = {"memcache1:11211", "memcache2:11211", "memcache3:11211"};
        mc::thread::client_t client(servers, scfg);

        // set the data
        client.set("szn", "seznam.cz");
        if (!client.prepend("szn", "http://"))
            throw std::runtime_error("prepend");
        if (!client.append("szn", "/"))
            throw std::runtime_error("append");

        // get the data
        auto res = client.get("szn");
        if (res) std::cout << res.data << std::endl;

    } catch (const mc::error_t &e) {
        std::cerr << "error: " << e.what() << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    return 0;
}
```

The default composition of the client class use the binary protocol that is
described here:
https://github.com/memcached/memcached/blob/master/doc/protocol-binary.xml

## Automatic conversion

The library supports automatic conversions for numeric types and google
protobufers. You can also extend the library for your own types.

```c++
#include <vector>
#include <string>
#include <iostream>

#include <mcache/mcache.h>

namespace app {

struct example_t { int value;};

} // namespace app

namespace mc {
namespace aux {

// atomatic conversion extension
template <>
struct cnv<app::example_t> {
    static app::example_t as(const std::string &data) {
        return {std::atoi(data.c_str())};
    }
    static std::string as(const app::example_t &data) {
        return std::to_string(data.value);
    }
};

} // namespace aux
} // namespace mc

int main(int, char *[]) {
    // library initialization
    mc::init();

    try {
        // configuration
        mc::server_proxy_config_t scfg;
        scfg.io_opts.timeouts.connect = 3000;
        scfg.io_opts.timeouts.write = 3000;
        scfg.io_opts.timeouts.read = 3000;

        // connect the servers pool
        std::vector<std::string>
            servers = {"memcache1:11211", "memcache2:11211", "memcache3:11211"};
        mc::thread::client_t client(servers, scfg);

        // set the data
        client.set("num", 3);
        client.set("class", app::example_t{3});

        // get the num
        auto res_num = client.get("num");
        if (res_num)
            std::cout << "num: " << res_num.as<int>() << std::endl;

        // get the object
        auto res_class = client.get("class");
        if (res_class)
            std::cout << "class: " << res_class.as<app::example_t>().value << std::endl;

    } catch (const mc::error_t &e) {
        std::cerr << "error: " << e.what() << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    return 0;
}
```

## Atomic updates on memcache server

The library provides simple mc::client_t::atomatic_update() function that use
cas (compare-and-swap) command for implementation of the atomic update of the
value on the memcache server.

```c++
#include <vector>
#include <string>
#include <iostream>

#include <mcache/mcache.h>

int main(int, char *[]) {
    // library initialization
    mc::init();

    try {
        // configuration
        mc::server_proxy_config_t scfg;
        scfg.io_opts.timeouts.connect = 3000;
        scfg.io_opts.timeouts.write = 3000;
        scfg.io_opts.timeouts.read = 3000;

        // connect the servers pool
        std::vector<std::string>
            servers = {"memcache1:11211", "memcache2:11211", "memcache3:11211"};
        mc::thread::client_t client(servers, scfg);

        // set the data
        client.set("key", "This is sentence.");

        // atomicly replace all spaces with underscores
        client.atomic_update("key",
        [] (std::string data, uint32_t flags) {
            std::replace(data.begin(), data.end(), ' ', '_');
            return std::make_pair(data, flags);
        });

        // get the data
        auto res = client.get("key");
        if (res) std::cout << res.data << std::endl;

    } catch (const mc::error_t &e) {
        std::cerr << "error: " << e.what() << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    return 0;
}
```

## Optional zlib compression

If you store bigger data, you can turn compression on via flags.

```c++
#include <vector>
#include <string>
#include <iostream>

#include <mcache/mcache.h>

int main(int, char *[]) {
    // library initialization
    mc::init();

    try {
        // configuration
        mc::server_proxy_config_t scfg;
        scfg.io_opts.timeouts.connect = 3000;
        scfg.io_opts.timeouts.write = 3000;
        scfg.io_opts.timeouts.read = 3000;

        // connect the servers pool
        std::vector<std::string>
            servers = {"memcache1:11211", "memcache2:11211", "memcache3:11211"};
        mc::thread::client_t client(servers, scfg);

        // set the data
        client.set("key", "very long string", {0, mc::opts_t::compress});

        // get the data
        auto res = client.get("key");
        if (res) std::cout << res.data << std::endl;

    } catch (const mc::error_t &e) {
        std::cerr << "error: " << e.what() << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    return 0;
}
```

# Python API

Python API is very similar to c++ one and contains automatic pickling for
python objects like you can see in following example.

```python
import mcache

client = mcache.Client(["memcache:11211"])

// objects
client.set("three", {"three": 3})
data = client.get("three")["data"]

// numbers
client.set("three", 3)
i = client.get("three")["data"]

// strings
client.set("three", "3")
s = client.get("three")["data"]

// options
client.set("three", "3", {"expiration" : 10})
```

