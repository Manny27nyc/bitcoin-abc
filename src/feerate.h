// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
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
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FEERATE_H
#define BITCOIN_FEERATE_H

#include <amount.h>
#include <serialize.h>

#include <cstdlib>
#include <ostream>
#include <string>
#include <type_traits>

/**
 * Fee rate in satoshis per kilobyte: Amount / kB
 */
class CFeeRate {
private:
    // unit is satoshis-per-1,000-bytes
    Amount nSatoshisPerK;

public:
    /**
     * Fee rate of 0 satoshis per kB.
     */
    constexpr CFeeRate() : nSatoshisPerK() {}
    explicit constexpr CFeeRate(const Amount _nSatoshisPerK)
        : nSatoshisPerK(_nSatoshisPerK) {}

    /**
     * Constructor for a fee rate in satoshis per kB. The size in bytes must not
     * exceed (2^63 - 1)
     */
    CFeeRate(const Amount nFeePaid, size_t nBytes);

    /**
     * Return the fee in satoshis for the given size in bytes.
     */
    Amount GetFee(size_t nBytes) const;

    /**
     * Return the ceiling of a fee calculation in satoshis for the given size in
     * bytes.
     */
    Amount GetFeeCeiling(size_t nBytes) const;

    /**
     * Return the fee in satoshis for a size of 1000 bytes
     */
    Amount GetFeePerK() const { return GetFee(1000); }

    /**
     * Equality
     */
    friend constexpr bool operator==(const CFeeRate a, const CFeeRate b) {
        return a.nSatoshisPerK == b.nSatoshisPerK;
    }
    friend constexpr bool operator!=(const CFeeRate a, const CFeeRate b) {
        return !(a == b);
    }

    /**
     * Comparison
     */
    friend bool operator<(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK < b.nSatoshisPerK;
    }
    friend bool operator>(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK > b.nSatoshisPerK;
    }
    friend bool operator<=(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK <= b.nSatoshisPerK;
    }
    friend bool operator>=(const CFeeRate &a, const CFeeRate &b) {
        return a.nSatoshisPerK >= b.nSatoshisPerK;
    }
    CFeeRate &operator+=(const CFeeRate &a) {
        nSatoshisPerK += a.nSatoshisPerK;
        return *this;
    }
    std::string ToString() const;

    SERIALIZE_METHODS(CFeeRate, obj) { READWRITE(obj.nSatoshisPerK); }
};

#endif // BITCOIN_FEERATE_H
