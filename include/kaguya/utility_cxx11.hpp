// Copyright satoren
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#pragma once

#include <string>

#include "kaguya/config.hpp"

namespace kaguya {
namespace util {
struct null_type {};

template <class... Args> struct TypeTuple {};
template <class Ret, class... Args> struct FunctionSignatureType {
  typedef Ret result_type;
  typedef TypeTuple<Args...> argument_type_tuple;
  static const size_t argument_count = sizeof...(Args);
  typedef Ret (*c_function_type)(Args...);
};
template <typename T> struct FunctorSignature {};

template <typename T, typename Ret, typename... Args>
struct FunctorSignature<Ret (T::*)(Args...) const> {
  typedef FunctionSignatureType<Ret, Args...> type;
};
template <typename T, typename Ret, typename... Args>
struct FunctorSignature<Ret (T::*)(Args...)> {
  typedef FunctionSignatureType<Ret, Args...> type;
};

#if defined(_MSC_VER) && _MSC_VER < 1900
template <typename T>
struct FunctionSignature : public FunctorSignature<decltype(&T::operator())> {};
#else

template <typename T, typename Enable = void> struct FunctionSignature;

template <typename T, typename = void>
struct has_operator_fn : std::false_type {};
template <typename T>
struct has_operator_fn<T, typename std::enable_if<!std::is_same<
                              void, decltype(&T::operator())>::value>::type>
    : std::true_type {};

template <typename T>
struct FunctionSignature<
    T, typename std::enable_if<has_operator_fn<T>::value>::type>
    : public FunctorSignature<decltype(&T::operator())> {};
#endif

template <typename T, typename Ret, typename... Args>
struct FunctionSignature<Ret (T::*)(Args...)> {
  typedef FunctionSignatureType<Ret, T &, Args...> type;
};
template <typename T, typename Ret, typename... Args>
struct FunctionSignature<Ret (T::*)(Args...) const> {
  typedef FunctionSignatureType<Ret, const T &, Args...> type;
};

#if defined(_MSC_VER) && _MSC_VER >= 1900 || defined(__cpp_ref_qualifiers)
template <typename T, typename Ret, typename... Args>
struct FunctionSignature<Ret (T::*)(Args...) const &> {
  typedef FunctionSignatureType<Ret, const T &, Args...> type;
};
template <typename T, typename Ret, typename... Args>
struct FunctionSignature<Ret (T::*)(Args...) const &&> {
  typedef FunctionSignatureType<Ret, const T &, Args...> type;
};
#endif

template <class Ret, class... Args> struct FunctionSignature<Ret (*)(Args...)> {
  typedef FunctionSignatureType<Ret, Args...> type;
};
template <class Ret, class... Args> struct FunctionSignature<Ret(Args...)> {
  typedef FunctionSignatureType<Ret, Args...> type;
};

template <typename F> struct FunctionResultType {
  typedef typename FunctionSignature<F>::type::result_type type;
};

template <std::size_t remain, class Arg, bool flag = remain <= 0>
struct TypeIndexGet;

template <std::size_t remain, class Arg, class... Args>
struct TypeIndexGet<remain, TypeTuple<Arg, Args...>, true> {
  typedef Arg type;
};

template <std::size_t remain, class Arg, class... Args>
struct TypeIndexGet<remain, TypeTuple<Arg, Args...>, false>
    : TypeIndexGet<remain - 1, TypeTuple<Args...> > {};
template <int N, typename F> struct ArgumentType {
  typedef typename TypeIndexGet<
      N, typename FunctionSignature<F>::type::argument_type_tuple>::type type;
};

namespace detail {
template <class F, class ThisType, class... Args>
auto invoke_helper(F &&f, ThisType &&this_, Args &&... args)
    -> decltype((std::forward<ThisType>(this_).*
                 f)(std::forward<Args>(args)...)) {
  return (std::forward<ThisType>(this_).*f)(std::forward<Args>(args)...);
}

template <class F, class... Args>
auto invoke_helper(F &&f, Args &&... args)
    -> decltype(f(std::forward<Args>(args)...)) {
  return f(std::forward<Args>(args)...);
}
}
template <class F, class... Args>
typename FunctionResultType<typename traits::decay<F>::type>::type
invoke(F &&f, Args &&... args) {
  return detail::invoke_helper(std::forward<F>(f), std::forward<Args>(args)...);
}

// -----------------------
// noexcept support helpers
// for c++17+
// Add forwarding specializations so noexcept function types are handled
// by the existing non-noexcept FunctionSignature implementations.
// Since c++17 kompilers distinguished functions: foo(); and foo() noexcept;
// so when the function is taged as noexcept the bindings does not recognize function name
// -----------------------

/* Free function types */
template <typename R, typename... Args>
struct FunctionSignature<R(Args...) noexcept, void>
  : FunctionSignature<R(Args...), void> {};

template <typename R, typename... Args>
struct FunctionSignature<R(*)(Args...) noexcept, void>
  : FunctionSignature<R(*)(Args...), void> {};

/* Member function pointers:
// Cover combinations of cv-qualifiers: none, const, volatile, const volatile
// and ref-qualifiers: none, &, &&
// Each noexcept variant forwards to the corresponding non-noexcept specialization.
*/

#define KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(cv_qual, ref_qual)                        \
template <typename R, typename C, typename... Args>                              \
struct FunctionSignature<R (C::*)(Args...) cv_qual ref_qual noexcept, void>       \
  : FunctionSignature<R (C::*)(Args...) cv_qual ref_qual, void> {};

#define KAGUYA_FORWARD_NOEXCEPT_MEMFUNC_CONST(cv_qual, ref_qual)                  \
template <typename R, typename C, typename... Args>                              \
struct FunctionSignature<R (C::*)(Args...) cv_qual ref_qual noexcept, void>       \
  : FunctionSignature<R (C::*)(Args...) cv_qual ref_qual, void> {};

// expand all common cv/ref combinations
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(, )
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(const, )
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(volatile, )
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(const volatile, )

KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(, &)
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(const, &)
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(volatile, &)
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(const volatile, &)

KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(, &&)
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(const, &&)
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(volatile, &&)
KAGUYA_FORWARD_NOEXCEPT_MEMFUNC(const volatile, &&)

#undef KAGUYA_FORWARD_NOEXCEPT_MEMFUNC
#undef KAGUYA_FORWARD_NOEXCEPT_MEMFUNC_CONST

// -----------------------
// End noexcept forwarding specializations
// -----------------------

}
}
