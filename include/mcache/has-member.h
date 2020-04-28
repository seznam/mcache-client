/*
 * FILE             $Id$
 *
 * DESCRIPTION      Member detection.
 *
 * PROJECT          Seznam memcache client.
 *
 * LICENSE          See COPYING
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2011
 * All Rights Reserved
 *
 * HISTORY
 *       2011-06-13 (bukovsky)
 *                  Stolen from szn-email-libszn.
 */

#ifndef MCACHE_HAS_MEMBER_H
#define MCACHE_HAS_MEMBER_H

#include <type_traits>

namespace mc {
namespace aux {

/** Macro which defines class has_MEMBER which can be used for member existence
 * detection.
 */
#define MCACHE_HAS_MEMBER(MEMBER)                                              \
template <typename Type_t, typename = void>                                    \
class has_ ## MEMBER {                                                         \
private:                                                                       \
    class yes { char m;};                                                      \
    class no { yes m[2];};                                                     \
                                                                               \
    /* fake call may override Type_t in Base_t */                              \
    struct Mixin_t { void MEMBER(){}};                                         \
    struct Base_t: public Type_t, public Mixin_t {};                           \
                                                                               \
    /* used as call existence detector */                                      \
    template <typename T, T t> class Detector_t {};                            \
                                                                               \
    /* if U does not have U::MEMBER member then this call will be used because
     * only Mixin_t::MEMBER is visible and possible because compiler
     * express more specialized methods */                                     \
    template <typename U>                                                      \
    static no deduce(U *, Detector_t<void (Mixin_t::*)(), &U::MEMBER> * = 0);  \
    /* fallback for all types which hash MEMBER member and for which
     * Detector_t class can't be instatiate */                                 \
    static yes deduce(...);                                                    \
                                                                               \
public:                                                                        \
    /* true if &Type_t::MEMBER is valid pointer */                             \
    enum { value = (sizeof(yes) == sizeof(deduce((Base_t *)(0))))};            \
};                                                                             \
                                                                               \
/* Specialization for non-class types that can't contains member... */         \
template <typename Type_t>                                                     \
class has_ ## MEMBER<Type_t, std::enable_if_t<!std::is_class<Type_t>::value>> {\
public:                                                                        \
    enum { value = 0};                                                         \
}                                                                              \

} // namespace aux
} // namespace mc

#endif /* MCACHE_HAS_MEMBER_H */

