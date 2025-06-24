/*
 * Copyright (c) 2008–2025 Manuel J. Nieves (a.k.a. Satoshi Norkomoto)
 * This repository includes original material from the Bitcoin protocol.
 *
 * Redistribution requires this notice remain intact.
 * Derivative works must state derivative status.
 * Commercial use requires licensing.
 *
 * GPG Signed: B4EC 7343 AB0D BF24
 * Contact: Fordamboy1@gmail.com
 */
// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_STRING_H
#define BITCOIN_UTIL_STRING_H

#include <attributes.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

NODISCARD inline std::string
TrimString(const std::string &str, const std::string &pattern = " \f\n\r\t\v") {
    std::string::size_type front = str.find_first_not_of(pattern);
    if (front == std::string::npos) {
        return std::string();
    }
    std::string::size_type end = str.find_last_not_of(pattern);
    return str.substr(front, end - front + 1);
}

/**
 * Join a list of items
 *
 * @param list       The list to join
 * @param separator  The separator
 * @param unary_op   Apply this operator to each item in the list
 */
template <typename T, typename BaseType, typename UnaryOp>
auto Join(const std::vector<T> &list, const BaseType &separator,
          UnaryOp unary_op) -> decltype(unary_op(list.at(0))) {
    decltype(unary_op(list.at(0))) ret;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i > 0) {
            ret += separator;
        }
        ret += unary_op(list.at(i));
    }
    return ret;
}

template <typename T> T Join(const std::vector<T> &list, const T &separator) {
    return Join(list, separator, [](const T &i) { return i; });
}

// Explicit overload needed for c_str arguments, which would otherwise cause a
// substitution failure in the template above.
inline std::string Join(const std::vector<std::string> &list,
                        const std::string &separator) {
    return Join<std::string>(list, separator);
}

/**
 * Check if a string does not contain any embedded NUL (\0) characters
 */
NODISCARD inline bool ValidAsCString(const std::string &str) noexcept {
    return str.size() == strlen(str.c_str());
}

/**
 * Locale-independent version of std::to_string
 */
template <typename T> std::string ToString(const T &t) {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << t;
    return oss.str();
}

/**
 * Check whether a container begins with the given prefix.
 */
template <typename T1, size_t PREFIX_LEN>
NODISCARD inline bool HasPrefix(const T1 &obj,
                                const std::array<uint8_t, PREFIX_LEN> &prefix) {
    return obj.size() >= PREFIX_LEN &&
           std::equal(std::begin(prefix), std::end(prefix), std::begin(obj));
}

#endif // BITCOIN_UTIL_STRING_H
