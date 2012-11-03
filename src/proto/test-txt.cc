/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: txt protocol test.
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

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/if.hpp>
#include <cxxabi.h>

#include <mcache/init.h>
#include <mcache/proto/txt.h>
#include <mcache/proto/response.h>
#include <mcache/proto/parser.h>

namespace test {

class validation_connection_error_t: public std::runtime_error {
public:
    validation_connection_error_t(const std::string &str)
        : std::runtime_error(str)
    {}
};

class validation_connection_t {
public:
    validation_connection_t(const std::string &request,
                            const std::string &header,
                            const std::string &body,
                            int delim = true)
        : delim(delim), request(request), responses()
    {
        responses.push_back(body);
        responses.push_back(header);
    }

    validation_connection_t(const std::string &request,
                            const std::string &header,
                            int delim = true)
        : delim(delim), request(request), responses()
    {
        responses.push_back(header);
    }

    void write(const std::string &validate) {
        if (validate != request)
            throw validation_connection_error_t("invalid request");
    }

    std::string read(const std::string &delimiter) {
        if (responses.empty())
            throw validation_connection_error_t("empty response");
        if (delim && !boost::ends_with(responses.back(), delimiter))
            throw validation_connection_error_t("invalid delimiter");
        std::string response = responses.back();
        responses.pop_back();
        return response;
    }

    std::string read(std::size_t size) {
        if (responses.empty())
            throw validation_connection_error_t("empty response");
        if (size != responses.back().size())
            throw validation_connection_error_t("invalid size");
        std::string response = responses.back();
        responses.pop_back();
        return response;
    }

    bool empty() const { return responses.empty();}

    bool delim;
    std::string request;
    std::vector<std::string> responses;
};

typedef mc::proto::command_parser_t<validation_connection_t> command_parser_t;
typedef mc::proto::txt::api api;

bool error_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "ERROR\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool error_client_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "CLIENT_ERROR <some error description>\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::client_error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool error_server_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "SERVER_ERROR <some error description>\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::server_error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool error_too_long_key() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command(std::string(251, '3'));
    const char request[] = "-";
    const char header[] = "-";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        parser.send(command);
        return false;

    } catch (const mc::error_t &e) {
        return e.code() == mc::err::bad_argument;

    } catch (const std::exception &e) {
        return false;
    }
    return false;
}

bool error_invalid_char_in_key() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3 3");
    const char request[] = "-";
    const char header[] = "-";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        parser.send(command);
        return false;

    } catch (const mc::error_t &e) {
        return e.code() == mc::err::bad_argument;

    } catch (const std::exception &e) {
        return false;
    }
    return false;
}

bool get_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "";
    validation_connection_t connection(request, header, false);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "invalid-response\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "ERROR\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "END\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "VALUE 3 0 3\r\n";
    const char body[] = "abc\r\nEND\r\n";
    validation_connection_t connection(request, header, body);

    // execute command
    try {
        command_parser_t parser(connection);
        if (!parser.send(command)) return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_found_flags() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "VALUE 3 123456 3\r\n";
    const char body[] = "abc\r\nEND\r\n";
    validation_connection_t connection(request, header, body);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).flags != 123456) return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_gets() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::gets_t command("3");
    const char request[] = "gets 3\r\n";
    const char header[] = "VALUE 3 123456 3 333\r\n";
    const char body[] = "abc\r\nEND\r\n";
    validation_connection_t connection(request, header, body);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).cas != 333) return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_invalid_body() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "VALUE 3 0 3\r\n";
    const char body[] = "abcddd\r\nEND\r\n";
    validation_connection_t connection(request, header, body);

    // execute command
    try {
        command_parser_t parser(connection);
        parser.send(command);
        return false;
    } catch (const validation_connection_error_t &) {{ return true;}
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_invalid_flags() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "VALUE 3 flags 3\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::syntax)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_invalid_size() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    const char request[] = "get 3\r\n";
    const char header[] = "VALUE 3 0 size\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::syntax)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc");
    const char request[] = "set 3 0 0 3\r\nabc\r\n";
    const char header[] = "";
    validation_connection_t connection(request, header, false);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc");
    const char request[] = "set 3 0 0 3\r\nabc\r\n";
    const char header[] = "invalid-response\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc");
    const char request[] = "set 3 0 0 3\r\nabc\r\n";
    const char header[] = "ERROR\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc");
    const char request[] = "set 3 0 0 3\r\nabc\r\n";
    const char header[] = "STORED\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::stored)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_not_stored() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc");
    const char request[] = "set 3 0 0 3\r\nabc\r\n";
    const char header[] = "NOT_STORED\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_stored)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_exists() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::cas_t command("3", "abc", mc::proto::opts_t(0, 0, 3));
    const char request[] = "cas 3 0 0 3 3\r\nabc\r\n";
    const char header[] = "EXISTS\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::exists)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::cas_t command("3", "abc", mc::proto::opts_t(0, 0, 3));
    const char request[] = "cas 3 0 0 3 3\r\nabc\r\n";
    const char header[] = "NOT_FOUND\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::incr_t command("3", 3);
    const char request[] = "incr 3 3\r\n";
    const char header[] = "";
    validation_connection_t connection(request, header, false);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::incr_t command("3", 3);
    const char request[] = "incr 3 3\r\n";
    const char header[] = "invalid-response\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::incr_t command("3", 3);
    const char request[] = "incr 3 3\r\n";
    const char header[] = "ERROR\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::incr_t command("3", 3);
    const char request[] = "incr 3 3\r\n";
    const char header[] = "33\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).data() != "33")
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::incr_t command("3", 3);
    const char request[] = "incr 3 3\r\n";
    const char header[] = "NOT_FOUND\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool touch_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::touch_t command("3", 3);
    const char request[] = "touch 3 3\r\n";
    const char header[] = "TOUCHED\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::touched)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    const char request[] = "delete 3\r\n";
    const char header[] = "";
    validation_connection_t connection(request, header, false);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    const char request[] = "delete 3\r\n";
    const char header[] = "invalid-response\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    const char request[] = "delete 3\r\n";
    const char header[] = "ERROR\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    const char request[] = "delete 3\r\n";
    const char header[] = "DELETED\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::deleted)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    const char request[] = "delete 3\r\n";
    const char header[] = "NOT_FOUND\r\n";
    validation_connection_t connection(request, header);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

class Checker_t {
public:
    Checker_t(): fails() {}

    void operator()(bool result) {
        fails += !result;
        if (result)
            std::cout << "[01;32m" << "ok" << "[01;0m" << std::endl;
        else
            std::cout << "[01;31m" << "fail" << "[01;0m" << std::endl;
    }

    int fails;
};

} // namespace test

int main(int, char **) {
    mc::init();
    test::Checker_t check;

    // error
    check(test::error_error());
    check(test::error_client_error());
    check(test::error_server_error());
    check(test::error_too_long_key());
    check(test::error_invalid_char_in_key());

    // retrieval
    check(test::get_command_empty());
    check(test::get_command_unrecognized());
    check(test::get_command_error());
    check(test::get_command_not_found());
    check(test::get_command_found());
    check(test::get_command_found_flags());
    check(test::get_command_gets());
    check(test::get_command_invalid_body());
    check(test::get_command_invalid_flags());
    check(test::get_command_invalid_size());

    // storage
    check(test::set_command_empty());
    check(test::set_command_unrecognized());
    check(test::set_command_error());
    check(test::set_command_ok());
    check(test::set_command_not_stored());
    check(test::set_command_not_found());
    check(test::set_command_exists());

    // incr/decr
    check(test::incr_command_empty());
    check(test::incr_command_unrecognized());
    check(test::incr_command_error());
    check(test::incr_command_ok());
    check(test::incr_command_not_found());
    check(test::touch_command_ok());

    // delete
    check(test::del_command_empty());
    check(test::del_command_unrecognized());
    check(test::del_command_error());
    check(test::del_command_ok());
    check(test::del_command_not_found());

    return check.fails;
}

