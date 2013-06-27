/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Declaration of conversion class.
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
 *       2012-11-05 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_CONVERSION_H
#define MCACHE_CONVERSION_H

#include <cstdio>
#include <limits>
#include <cstdlib>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_float.hpp>

#include <mcache/error.h>
#include <mcache/has-member.h>

namespace mc {
namespace aux {

// Macro fot detection of as() member function.
MCACHE_HAS_MEMBER(as);

/** Fallback uninplemented function.
 */
template <typename> inline const char *format();

/// format for special int types
template <> inline const char *format<bool>() { return "%d\0";}
template <> inline const char *format<char>() { return "%d\0";}
template <> inline const char *format<wchar_t>() { return "%d\0";}

/// format for signed int types
template <> inline const char *format<int8_t>() { return "%d\0";}
template <> inline const char *format<int16_t>() { return "%d\0";}
template <> inline const char *format<int32_t>() { return "%d\0";}
template <> inline const char *format<int64_t>() { return "%ld\0";}
template <> inline const char *format<long long int>() { return "%lld\0";}

/// format for unsigned int types
template <> inline const char *format<uint8_t>() { return "%u\0";}
template <> inline const char *format<uint16_t>() { return "%u\0";}
template <> inline const char *format<uint32_t>() { return "%u\0";}
template <> inline const char *format<uint64_t>() { return "%lu\0";}
template <> inline const char *format<unsigned long long int>() {
    return "%llu\0";
}

/// format for float types
template <> inline const char *format<float>() { return "%f\0";}
template <> inline const char *format<double>() { return "%f\0";}
template <> inline const char *format<long double>() { return "%Lf\0";}

/** Conversion class that are used as fallback for all conversions.
 *
 * If you see error here you must define your own serialization/deserialization
 * specialization of this class as you can see bellow.
 *
 * This class should keep not implemented to generate nice errors when one use
 * unimplemented conversion.
 */
template <typename type_t, typename = void>
struct cnv {};

/** Conversions for integral types. That allows this code:
 *
 * int three = 3;
 * client.set("key", three);
 * three = client.get("key").as<int>();
 */
template <typename integral_t>
struct cnv<
           integral_t,
           typename boost::enable_if<boost::is_integral<integral_t> >::type
       > {
    static integral_t as(const std::string &data) {
        return static_cast<integral_t>(::atoll(data.c_str()));
    }
    static std::string as(integral_t data) {
        char buffer[4 * sizeof(integral_t) + 1];
        ::snprintf(buffer, sizeof(buffer), format<integral_t>(), data);
        return buffer;
    }
};

/** Conversions for float types. That allows this code:
 *
 * double pi = 3.14;
 * client.set("key", pi);
 * pi = client.get("key").as<double>();
 */
template <typename float_t>
struct cnv<
           float_t,
           typename boost::enable_if<boost::is_float<float_t> >::type
       > {
    static float_t as(const std::string &data) {
        return static_cast<float_t>(::strtold(data.c_str(), 0));
    }
    static std::string as(float_t data) {
        char buffer[std::numeric_limits<float_t>::max_exponent10 + 20];
        ::snprintf(buffer, sizeof(buffer), format<float_t>(), data);
        return buffer;
    }
};

/// Macros for detection of FRPC::Value_t types.
MCACHE_HAS_MEMBER(marshallToFRPCBinaryStream);

/** Conversions for FRPC values. That allows this code:
 *
 * FRPC::Pool_t pool;
 * FRPC::Value_t &fvalue = pool.Int(3);
 * client.set("key", fvalue);
 * FRPC::Int_t &fint = FRPC::Int(client.get("key").as<FRPC::Value_t>(pool));
 */
template <typename frpc_value_t>
struct cnv<
           frpc_value_t,
           typename boost::enable_if_c<
               has_marshallToFRPCBinaryStream<frpc_value_t>::value
           >::type
       > {
    template <typename pool_t>
    static const frpc_value_t &as(pool_t &pool, const std::string &data) {
        return pool.demarshallFromFRPCBinaryStream(data);
    }
    static std::string as(const frpc_value_t &value) {
        return value.marshallToFRPCBinaryStream();
    }
};

/// Macros for detection of FRPC::Value_t types.
MCACHE_HAS_MEMBER(SerializeToString);
MCACHE_HAS_MEMBER(ParseFromString);

/** Conversions for FRPC values. That allows this code:
 *
 * FRPC::Pool_t pool;
 * FRPC::Value_t &fvalue = pool.Int(3);
 * client.set("key", fvalue);
 * FRPC::Int_t &fint = FRPC::Int(client.get("key").as<FRPC::Value_t>(pool));
 */
template <typename protobuf_t>
struct cnv<
           protobuf_t,
           typename boost::enable_if_c<
               has_SerializeToString<protobuf_t>::value
               && has_ParseFromString<protobuf_t>::value
           >::type
       > {
    static protobuf_t as(const std::string &data) {
        protobuf_t protobuf;
        if (!protobuf.ParseFromString(data) || !protobuf.IsInitialized())
            throw error_t(err::bad_argument, "can't deserialize protobuf");
        return protobuf;
    }
    static std::string as(const protobuf_t &protobuf) {
        std::string data;
        if (!protobuf.IsInitialized() || !protobuf.SerializeToString(data))
            throw error_t(err::bad_argument, "can't serialize protobuf");
        return data;
    }
};

} // namespace aux
} // namespace mc

#endif /* MCACHE_CONVERSION_H */

