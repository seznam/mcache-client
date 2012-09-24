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

#ifdef HAVE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#else
#error install libreadline and reconfigure
#endif /* HAVE_READLINE_H */

#include <mcache/init.h>
#include <mcache/io/connection.h>

class fake_get_command_t {
public:
    fake_get_command_t(const std::string &data): data(data + "\r\n") {}
    const std::string &serialize() const { return data;}
    std::string header_delimiter() const { return "\r\n";}
    mc::response_t deserialize_header(const std::string &header) const {
        std::cout << "HEADER " << header << std::endl;
        return mc::response_t(header);
    }

    std::string data;
};

bool exec(mc::io::tcp::connection_t &connection, const std::string &line) {
    // help
    if ((boost::starts_with(line, "help")) || (boost::starts_with(line, "?"))) {
        std::cout << "help,?\tprints this help\n"
                  << "quit,q\texit the program\n"
                  << "send\traw send to remote server\n"
                  << std::endl;
        return false;
    }

    // quit
    if ((boost::starts_with(line, "quit")) || (boost::starts_with(line, "q"))) {
        return true;
    }

    // send
    if (boost::starts_with(line, "send")) {
        std::cout << "Now you can write memcache command in memcache protocol."
                  << "\nYou can finish editing command by entering line with "
                  << "single dot:" << std::endl;
        std::string data;
        std::string prompt = ">>> ";
        for (;;) {
            char *chunk;
            if (!(chunk = ::readline(prompt.c_str()))) break;
            if (std::string(chunk) == ".") break;
            data += chunk;
            ::free(chunk);
        }
        ::add_history(data.c_str());
        connection.send(fake_get_command_t(data));
        return false;
    }

    // unknown
    std::cout << "unknown command: " << line << std::endl << std::endl;
    return false;
}

int main(int argc, char **argv) {
    // read destionation address
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " dest-server:port\n\n"
                  << "\tMichal Bukovsky <michal.bukovsky@firma.seznam.cz>\n"
                  << "\tCopyright (C) Seznam.cz a.s. 2012\n";
        return EXIT_FAILURE;
    }
    std::string dst = argv[1];

    // establish connection to server
    mc::init();
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
        std::string cmd = boost::trim_copy(std::string(line));
        ::add_history(cmd.c_str());
        if (exec(connection, cmd)) break;
        ::free(line);
    }
    ::write_history(histfile.c_str());
    ::history_truncate_file(histfile.c_str(), 1000);
}

