/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Python module.
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
 *       2012-11-19 (bukovsky)
 *                  First draft.
 */

#include <string>
#include <vector>
#include <boost/python.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <mcache/init.h>
#include <mcache/mcache.h>
#include <mcache/conversion.h>

// The documentation isn't great, but the name 'borrowed' is a hint.  Using
// borrowed instructs boost.python that the PyObject * is a borrowed
// reference.  So you can use:
//
// object(handle<>(borrowed(ptr)))
//
// for when ptr is a borrowed reference and
//
// object(handle<>(ptr))
//
// for when ptr is a new reference.

namespace mc {
namespace py {
namespace {

#if PY_VERSION_HEX >= 0x03000000
#define MC_PyString_Check(O) (PyUnicode_Check(O))
#else /* PY_VERSION_HEX */
#define MC_PyString_Check(O) (PyString_Check(O))
#endif /* PY_VERSION_HEX */

#if PY_VERSION_HEX >= 0x02060000
#define MC_PyBytes_FromStringAndSize(A, B) (PyBytes_FromStringAndSize(A, B))
#define MC_PyBytes_AsStringAndSize(O, A, B) PyBytes_AsStringAndSize(O, A, B)
#define MC_PyBytes_Check(O) PyBytes_Check(O)
#else /* PY_VERSION_HEX */
#define MC_PyBytes_FromStringAndSize(A, B) (PyString_FromStringAndSize(A, B))
#define MC_PyBytes_AsStringAndSize(O, A, B) PyString_AsStringAndSize(O, A, B)
#define MC_PyBytes_Check(O) PyString_Check(O)
#endif /* PY_VERSION_HEX */

/** Tries import module and if module was not found then None is returned.
 */
static boost::python::object try_import(const char *module) {
    try {
        return boost::python::import(module);

    } catch (const boost::python::error_already_set &) {
        if (!PyErr_ExceptionMatches(PyExc_ImportError)) throw;
        PyErr_Clear();
    }
    return boost::python::object();
}

/** Returns std::string as python 'bytes' object.
 */
static boost::python::object as_python_bytes(const std::string &bytes) {
    boost::python::handle<>
        handle(MC_PyBytes_FromStringAndSize(bytes.data(), bytes.size()));
    return boost::python::object(handle);
}

/** Sets result from entry in dict if exists.
 */
template <typename type_t>
static void set_from(type_t &res,
                     const boost::python::object &dict,
                     const char *name)
{
    if (dict.contains(name)) res = boost::python::extract<type_t>(dict[name]);
}


template <typename client_t>
class atomic_update_fn_t {
public:
    atomic_update_fn_t(client_t *client,
            boost::python::object fn) :
        client(client), fn(fn)
    {}

    std::pair<std::string, uint32_t>
    operator()(const std::string &data, uint32_t flags) {
        boost::python::object newdata;

        if (data.empty() && flags == 0) {
            newdata = fn(boost::python::object());
        } else {
            boost::python::object pydata =
                client->data_from_string(data, flags);
            newdata = fn(pydata);
        }

        std::pair<std::string, uint32_t> rv;
        mc::opts_t o;
        rv.first = client->to_string(newdata, o);
        rv.second = o.flags;
        return rv;
    }

    client_t *client;
    boost::python::object fn;
};


} // namespace

/** Python wrapper around memcache client.
 */
template <typename impl_t>
class client_t {
public:

    /** Flags that are used as type mark.
     */
    enum {
        STRING  = 0x00000000,
        PICKLED = 0x20000000,
        DOUBLE  = 0x40000000,
        LONG    = 0x80000000,
#if PY_VERSION_HEX < 0x03000000
        INTEGER = 0xC0000000,
#endif /* PY_VERSION_HEX */
        MASK    = 0xE0000000
    };

    /** C'tor.
     */
    client_t(const boost::python::object &o,
             const boost::python::dict &dict = boost::python::dict())
        : client(), loads(), dumps()
    {
        // prepare options
        mc::server_proxy_config_t scfg;
        mc::consistent_hashing_pool_config_t pcfg;
        mc::client_config_t ccfg;
        set_from(scfg.io_opts.timeouts.connect, dict, "connect_timeout");
        set_from(scfg.io_opts.timeouts.read, dict, "read_timeout");
        set_from(scfg.io_opts.timeouts.write, dict, "write_timeout");
        set_from(scfg.fail_limit, dict, "restoration_fail_limit");
        set_from(scfg.restoration_interval, dict, "restoration_interval");
        set_from(pcfg.virtual_nodes, dict, "virtual_nodes");
        set_from(ccfg.max_continues, dict, "max_continues");
        set_from(ccfg.h404_duration, dict, "h404_duration");

        // convert to vector
        boost::python::stl_input_iterator<std::string> begin(o);
        boost::python::stl_input_iterator<std::string> end;
        std::vector<std::string> addresses(begin, end);
        client = boost::make_shared<impl_t>(addresses, scfg, pcfg, ccfg);

        // pickle module
        boost::python::object pickle = try_import("cPickle");
        if (!pickle) pickle = boost::python::import("pickle");
        dumps = pickle.attr("dumps");
        loads = pickle.attr("loads");
    }

    /** Returns dict for not_found result.
     */
    boost::python::object not_found(boost::python::object def) {
        boost::python::dict dict;
        dict["cas"] = 0;
        dict["flags"] = 0;
        dict["data"] = def;
        return dict;
    }

    /** Converts python object to string that will be stored on memcache server.
     */
    std::string to_string(const boost::python::object &data, opts_t &opts) {
        // check our bits
        if (opts.flags & MASK) {
            throw mc::error_t(mc::err::bad_argument,
                              "three upper bits of flags are "
                              "used by mcache python wrapper");
        }

        // string
        if (MC_PyString_Check(data.ptr())) {
            return boost::python::extract<std::string>(data);

        // float
        } else if (PyFloat_Check(data.ptr())) {
            opts.flags |= DOUBLE;
            return aux::cnv<double>::as(boost::python::extract<double>(data));

#if PY_VERSION_HEX < 0x03000000
        // int
        } else if (PyInt_Check(data.ptr())) {
            opts.flags |= INTEGER;
            return aux::cnv<int32_t>::as(boost::python::extract<int32_t>(data));
#endif /* PY_VERSION_HEX */

        // long
        } else if (PyLong_Check(data.ptr())) {
            opts.flags |= LONG;
            return aux::cnv<int64_t>::as(boost::python::extract<int64_t>(data));

        }
        opts.flags |= PICKLED;
        return std::string(boost::python::extract<std::string>(dumps(data)));
    }


    boost::python::object
    data_from_string(const std::pair<std::string, uint32_t> &result) {
        return data_from_string(result.first, result.second);
    }

    /** Converts string fetched from memcache server to python object.
     */
    boost::python::object
    data_from_string(const std::string &data, uint32_t flags) {
        boost::python::object rv;
        switch (flags & MASK) {
        case PICKLED:
            rv = loads(as_python_bytes(data));
            break;
        case DOUBLE:
            rv = boost::python::object(aux::cnv<double>::as(data));
            break;
#if PY_VERSION_HEX < 0x03000000
        case INTEGER:
            rv = boost::python::object(aux::cnv<int32_t>::as(data));
            break;
#endif /* PY_VERSION_HEX */
        case LONG:
            rv = boost::python::object(aux::cnv<int64_t>::as(data));
            break;
        case STRING:
        default:
            rv = boost::python::object(data);
            break;
        }
        return rv;
    }

    /** Converts string fetched from memcache server to python result object.
     */
    boost::python::object
    from_string(mc::result_t result,
                boost::python::object def = boost::python::object())
    {
        // call and solve not found as none
        if (!result) return not_found(def);

        // extract data
        boost::python::object
            data = data_from_string(result.data, result.flags);

        // return result
        boost::python::dict dict;
        dict["cas"] = result.cas;
        dict["flags"] = result.flags & ~MASK;
        dict["data"] = data;
        return dict;
    }

    // \defgroup public_api Public api.
    // @{
    void set(const std::string &key, const boost::python::object &data) {
        seto(key, data, opts_t());
    }

    void seto(const std::string &key,
              const boost::python::object &data,
              opts_t opts)
    {
        client->set(key, to_string(data, opts), opts);
    }

    bool add(const std::string &key, const boost::python::object &data) {
        return addo(key, data, opts_t());
    }

    bool addo(const std::string &key,
              const boost::python::object &data,
              opts_t opts)
    {
        return client->add(key, to_string(data, opts), opts);
    }

    bool replace(const std::string &key, const boost::python::object &data) {
        return replaceo(key, data, opts_t());
    }

    bool replaceo(const std::string &key,
                  const boost::python::object &data,
                  opts_t opts)
    {
        return client->replace(key, to_string(data, opts), opts);
    }

    bool prepend(const std::string &key, const boost::python::object &data) {
        return prependo(key, data, opts_t());
    }

    bool prependo(const std::string &key,
                  const boost::python::object &data,
                  opts_t opts)
    {
        return client->prepend(key, to_string(data, opts), opts);
    }

    bool append(const std::string &key, const boost::python::object &data) {
        return appendo(key, data, opts_t());
    }

    bool appendo(const std::string &key,
                 const boost::python::object &data,
                 opts_t opts)
    {
        return client->append(key, to_string(data, opts), opts);
    }

    bool cas(const std::string &key,
             const boost::python::object &data,
             opts_t opts)
    {
        return client->cas(key, to_string(data, opts), opts);
    }

    boost::python::object get(const std::string &key) {
        return from_string(client->get(key));
    }

    boost::python::object gets(const std::string &key) {
        return from_string(client->gets(key));
    }

    boost::python::object
    getd(const std::string &key, boost::python::object def) {
        try {
            return from_string(client->get(key), def);
        } catch (const mc::out_of_servers_t &) { return  not_found(def);}
    }

    boost::python::object
    getsd(const std::string &key, boost::python::object def) {
        try {
            return from_string(client->gets(key), def);
        } catch (const mc::out_of_servers_t &) { return  not_found(def);}
    }

    boost::python::object incr(const std::string &key) {
        return incrio(key, 1, opts_t());
    }

    boost::python::object incri(const std::string &key, uint64_t inc) {
        return incrio(key, inc, opts_t());
    }

    boost::python::object incrio(const std::string &key,
                                 uint64_t inc,
                                 opts_t opts)
    {
        opts.flags |= LONG;
        std::pair<uint64_t, bool> pair = client->incr(key, inc, opts);
        if (!pair.second) return boost::python::object();
        return boost::python::object(pair.first);
    }

    boost::python::object decr(const std::string &key) {
        return decrio(key, 1, opts_t());
    }

    boost::python::object decri(const std::string &key, uint64_t dec) {
        return decrio(key, dec, opts_t());
    }

    boost::python::object decrio(const std::string &key,
                                 uint64_t dec,
                                 opts_t opts)
    {
        opts.flags |= LONG;
        std::pair<uint64_t, bool> pair = client->decr(key, dec, opts);
        if (!pair.second) return boost::python::object();
        return boost::python::object(pair.first);
    }

    bool touch(const std::string &key, uint64_t exp) {
        return client->touch(key, exp);
    }

    bool del(const std::string &key) { return client->del(key);}

    boost::python::object atomic_updateo(const std::string &key,
            boost::python::object fn, const opts_t &opts) {

        atomic_update_fn_t<client_t<impl_t> > pyfn(this, fn);

        std::pair<std::string, uint32_t> rv =
            client-> template atomic_update(key, pyfn, opts);

        return data_from_string(rv);
    }

    boost::python::object atomic_update(const std::string &key,
            boost::python::object fn) {
        return atomic_updateo(key, fn, opts_t());
    }

    // @}

    /** Register client object methods.
     */
    static void define(const char *name) {
        boost::python::class_<client_t>

            (name, boost::python::init<
                       boost::python::object,
                       boost::python::optional<boost::python::dict>
                   >())

            .def("set", &client_t::seto)
            .def("set", &client_t::set)
            .def("add", &client_t::add)
            .def("add", &client_t::addo)
            .def("replace", &client_t::replace)
            .def("replace", &client_t::replaceo)
            .def("prepend", &client_t::prepend)
            .def("prepend", &client_t::prependo)
            .def("append", &client_t::append)
            .def("append", &client_t::appendo)
            .def("cas", &client_t::cas)
            .def("get", &client_t::get)
            .def("get", &client_t::getd)
            .def("gets", &client_t::gets)
            .def("gets", &client_t::getsd)
            .def("incr", &client_t::incrio)
            .def("incr", &client_t::incr)
            .def("incr", &client_t::incri)
            .def("decr", &client_t::decrio)
            .def("decr", &client_t::decr)
            .def("decr", &client_t::decri)
            .def("touch", &client_t::touch)
            .def("delete", &client_t::del)
            .def("atomic_update", &client_t::atomic_update)
            .def("atomic_update", &client_t::atomic_updateo);
    }

private:
    boost::shared_ptr<impl_t> client; //!< memcache client
    boost::python::object loads;      //!< pickle.loads function
    boost::python::object dumps;      //!< pickle.dumps function
};

/** Allow converts python 'bytes' type to std::string.
 */
struct std_string_from_python_bytes {
public:
    /** C'tor.
     */
    std_string_from_python_bytes() {
#if PY_VERSION_HEX >= 0x03000000
        boost::python::converter::registry
        ::push_back(&convertible,
                    &construct,
                    boost::python::type_id<std::string>());
#endif /* PY_VERSION_HEX */
    }

    /** Return non NULL if o matches 'bytes' type.
     */
    static void *convertible(PyObject *o) { return MC_PyBytes_Check(o)? o: 0x0;}

    /** Converts object into a std::string.
     */
    static void construct(PyObject *o,
                          boost::python::converter
                          ::rvalue_from_python_stage1_data *data)
    {
        // extract the bytes data from the python
        char *buffer = 0x0;
        Py_ssize_t size = 0;
        if (MC_PyBytes_AsStringAndSize(o, &buffer, &size) == -1)
            throw boost::python::error_already_set();

        // grab pointer to memory into which to construct the new std::string
        void *storage = ((boost::python::converter
                          ::rvalue_from_python_storage<std::string> *)data)
                        ->storage.bytes;

        // in-place construct the new std::string
        new (storage) std::string(buffer, size);

        // stash the memory chunk pointer for later use by boost.python
        data->convertible = storage;
    }
};

/** Allow converts python dict type to mc::opts_t.
 */
struct opts_from_python_dict {
public:
    /** C'tor.
     */
    opts_from_python_dict() {
        boost::python::converter::registry
        ::push_back(&convertible,
                    &construct,
                    boost::python::type_id<mc::opts_t>());
    }

    /** Return non NULL if o matches dict type.
     */
    static void *convertible(PyObject *o) { return PyDict_Check(o)? o: 0x0;}

    /** Converts object into a std::string.
     */
    static void construct(PyObject *o,
                          boost::python::converter
                          ::rvalue_from_python_stage1_data *data)
    {
        // extract data from dict
        time_t expiration = 0;
        uint32_t flags = 0;
        uint64_t cas = 0;
        boost::python::object dict(boost::python::borrowed<>(o));
        set_from(expiration, dict, "expiration");
        set_from(flags, dict, "flags");
        if (dict.contains("cas")) set_from(cas, dict, "cas");
        else if (dict.contains("iters")) set_from(cas, dict, "iters");
        else set_from(cas, dict, "initial");

        // grab pointer to memory into which to construct the new mc::opts_t
        void *storage = ((boost::python::converter
                          ::rvalue_from_python_storage<mc::opts_t> *)data)
                        ->storage.bytes;

        // in-place construct the new mc::opts_t
        new (storage) mc::opts_t(expiration, flags, cas);

        // stash the memory chunk pointer for later use by boost.python
        data->convertible = storage;
    }
};

} // namespace py
} // namespace mc

BOOST_PYTHON_MODULE(mcache) {
    mc::init();
    mc::py::std_string_from_python_bytes();
    mc::py::opts_from_python_dict();
    mc::py::client_t<mc::ipc::client_t>::define("Client");
    mc::py::client_t<mc::ipc::udp::client_t>::define("UDPClient");
}

