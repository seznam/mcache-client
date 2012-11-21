/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: io test program.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-21 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/function.hpp>

#ifdef HAVE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#else
#error install libreadline and reconfigure
#endif /* HAVE_READLINE_H */

#include <mcache/init.h>
#include <mcache/io/connection.h>
#include <mcache/proto/txt.h>
#include <mcache/proto/binary.h>
#include <mcache/proto/parser.h>

namespace {

class dispatcher_t {
public:
    typedef std::map<
                std::string,
                boost::function<bool (mc::io::tcp::connection_t &,
                                      const std::string &)>
            > handlers_t;

    handlers_t::value_type::second_type
    operator[](const std::string &cmd) const {
        using boost::lambda::constant;
        handlers_t::const_iterator ihandler = handlers.find(cmd);
        if (ihandler != handlers.end()) return ihandler->second;
        return ((std::cout << constant("unknown command: ")
                           << constant(cmd) << '\n'),
                constant(false));
    }

    void insert(const std::string &cmd,
                handlers_t::value_type::second_type func)
    {
        handlers.insert(std::make_pair(cmd, func));
    }

    handlers_t handlers;
};

template <typename command_t>
class retrieve_t {
public:
    bool operator()(mc::io::tcp::connection_t &connection,
                    const std::string &line)
    {
        typedef mc::proto::command_parser_t<mc::io::tcp::connection_t> parser_t;
        parser_t parser(connection);
        typename command_t::response_t response =
            parser.send(command_t(boost::trim_copy(line)));
        if (!response) throw response.exception();
        std::cout << "response = {" << std::endl
                  << "    status = " << response.code() << std::endl
                  << "    flags = " << response.flags << std::endl
                  << "    cas = " << response.cas << std::endl
                  << "    data-size = " << response.data().size() << std::endl
                  << "    data = \"" << response.data() << "\"" << std::endl
                  << "}" << std::endl;
        return false;
    }
};

template <typename command_t>
class storage_t {
public:
    bool operator()(mc::io::tcp::connection_t &connection,
                    const std::string &line)
    {
        // parse
        std::istringstream is(line);
        std::string key;
        std::string data;
        uint64_t expiration = 0;
        uint32_t flags = 0;
        uint64_t cas = 0;
        is >> key >> data >> expiration >> flags >> cas;
        mc::proto::opts_t opts(expiration, flags, cas);

        // run
        typedef mc::proto::command_parser_t<mc::io::tcp::connection_t> parser_t;
        parser_t parser(connection);
        typename command_t::response_t response =
            parser.send(command_t(key, data, opts));
        if (!response) throw response.exception();
        std::cout << "response = {" << std::endl
                  << "    status = " << response.code() << std::endl
                  << "}" << std::endl;
        return false;
    }
};

template <typename command_t>
class incr_decr_t {
public:
    bool operator()(mc::io::tcp::connection_t &connection,
                    const std::string &line)
    {
        // parse
        std::istringstream is(line);
        std::string key;
        uint64_t value = 1;
        uint64_t initial = 0;
        is >> key >> value >> initial;

        // run
        typedef mc::proto::command_parser_t<mc::io::tcp::connection_t> parser_t;
        parser_t parser(connection);
        mc::proto::opts_t opts(0, 0, initial);
        typename command_t::response_t
            response = parser.send(command_t(key, value, opts));
        if (!response) throw response.exception();
        std::cout << "response = {" << std::endl
                  << "    status = " << response.code() << std::endl
                  << "    new-value = " << response.data() << std::endl
                  << "}" << std::endl;
        return false;
    }
};

template <>
class incr_decr_t<mc::proto::bin::api::touch_t> {
public:
    bool operator()(mc::io::tcp::connection_t &connection,
                    const std::string &line)
    {
        // parse
        std::istringstream is(line);
        std::string key;
        time_t value = 0;
        is >> key >> value;

        // run
        typedef mc::proto::bin::api::touch_t command_t;
        typedef mc::proto::command_parser_t<mc::io::tcp::connection_t> parser_t;
        parser_t parser(connection);
        typename command_t::response_t response =
            parser.send(command_t(key, value));
        if (!response) throw response.exception();
        std::cout << "response = {" << std::endl
                  << "    status = " << response.code() << std::endl
                  << "}" << std::endl;
        return false;
    }
};

template <typename command_t>
class delete_t {
public:
    bool operator()(mc::io::tcp::connection_t &connection,
                    const std::string &line)
    {
        typedef mc::proto::command_parser_t<mc::io::tcp::connection_t> parser_t;
        parser_t parser(connection);
        typename command_t::response_t response =
            parser.send(command_t(boost::trim_copy(line)));
        if (!response) throw response.exception();
        std::cout << "response = {" << std::endl
                  << "    status = " << response.code() << std::endl
                  << "}" << std::endl;
        return false;
    }
};

std::pair<std::string, std::string> split(const std::string &line) {
    std::string word;
    std::istringstream is(line);
    is >> word;
    return std::make_pair(word, line.substr(word.size()));
}

template <typename api>
void register_methods(dispatcher_t &dispatcher) {
    dispatcher.insert("get", retrieve_t<typename api::get_t>());
    dispatcher.insert("gets", retrieve_t<typename api::gets_t>());
    dispatcher.insert("set", storage_t<typename api::set_t>());
    dispatcher.insert("add", storage_t<typename api::add_t>());
    dispatcher.insert("replace", storage_t<typename api::replace_t>());
    dispatcher.insert("append", storage_t<typename api::append_t>());
    dispatcher.insert("prepend", storage_t<typename api::prepend_t>());
    dispatcher.insert("cas", storage_t<typename api::cas_t>());
    dispatcher.insert("incr", incr_decr_t<typename api::incr_t>());
    dispatcher.insert("decr", incr_decr_t<typename api::decr_t>());
    dispatcher.insert("del", delete_t<typename api::delete_t>());
    dispatcher.insert("touch", incr_decr_t<typename api::touch_t>());
}

static const char
HELP[] = "help\t\t\t\t\tprints this help\n"
         "quit,q\t\t\t\t\texit the program\n"
         "get <key>\t\t\t\tget value\n"
         "set <key> <value> <exp> <flags>\t\tset value\n"
         "set <key> <value> <exp>\t\t\tset value\n"
         "set <key> <value>\t\t\tset value\n"
         "add <key> <value>\t\t\tset value\n"
         "replace <key> <value>\t\t\tset value\n"
         "append <key> <value>\t\t\tappend the value after stored value\n"
         "prepend <key> <value>\t\t\tprepend the value before stored value\n"
         "cas <key> <value> <exp> <flags> <cas>\tset value and check CAS id\n"
         "incr <key> <value> <initial>\t\tincrement value by value\n"
         "incr <key> <value>\t\t\tincrement value by value\n"
         "incr <key>\t\t\t\tincrement value\n"
         "decr <key> <value>\t\t\tdecrement value by value\n"
         "decr <key>\t\t\t\tdecrement value\n"
         "del <key>\t\t\t\tdelete value\n"
         "touch <key> <value>\t\t\ttouch value timestamp\n";

} // namespace

int main(int argc, char **argv) {
    mc::init();
    using boost::lambda::_1;
    using boost::lambda::_2;
    using boost::lambda::constant;

    // read destionation address
    std::string dst;
    bool binary = false;
    if (argc == 3) {
        if (strcmp(argv[1], "-b") == 0) {
            binary = true;
            dst = argv[2];

        } else if (strcmp(argv[2], "-b") == 0) {
            binary = true;
            dst = argv[1];

        } else if (strcmp(argv[1], "-t") == 0) {
            binary = false;
            dst = argv[2];

        } else if (strcmp(argv[2], "-t") == 0) {
            binary = false;
            dst = argv[1];

    } else if (argc == 2) dst = argv[1];

    // is destionation server set properly
    if (dst.empty()) {
        std::cerr << "Usage: " << argv[0] << " dest-server:port\n\n"
                  << "\tMichal Bukovsky <michal.bukovsky@firma.seznam.cz>\n"
                  << "\tCopyright (C) Seznam.cz a.s. 2012\n";
        return EXIT_FAILURE;
    }

    // prepare dispatch table
    dispatcher_t dispatcher;
    dispatcher.insert("quit", constant(true));
    dispatcher.insert("q", constant(true));
    dispatcher.insert("exit", constant(true));
    dispatcher.insert("help", (std::cout << constant(HELP), constant(false)));
    if (binary) register_methods<mc::proto::bin::api>(dispatcher);
    else register_methods<mc::proto::txt::api>(dispatcher);

    // establish connection to server
    mc::io::tcp::connection_t connection(dst, mc::io::opts_t());

    // main loop
    std::string prompt = dst + "> ";
    std::string home = ::getenv("HOME");
    std::string histfile = home + "/.libmcache-test-connection.history";
    ::rl_readline_name = "libmcache-test-connection";
    ::read_history(histfile.c_str());
    for (;;) {
        char *line;
        if (!(line = ::readline(prompt.c_str()))) break;
        try {
            std::pair<std::string, std::string>
                cmd = split(boost::trim_copy(std::string(line)));
            ::add_history((cmd.first + cmd.second).c_str());
            if (dispatcher[cmd.first](connection, cmd.second)) break;
        } catch (const std::exception &e) {
            connection = mc::io::tcp::connection_t(dst, mc::io::opts_t());
            std::cout << e.what() << std::endl;
        }
        ::free(line);
    }
    ::write_history(histfile.c_str());
    ::history_truncate_file(histfile.c_str(), 1000);
}

