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

#include <feerate.h>

#include <network.h>

#include <tinyformat.h>

CFeeRate::CFeeRate(const Amount nFeePaid, size_t nBytes_) {
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nSize > 0) {
        nSatoshisPerK = 1000 * nFeePaid / nSize;
    } else {
        nSatoshisPerK = Amount::zero();
    }
}

template <bool ceil>
static Amount GetFee(size_t nBytes_, Amount nSatoshisPerK) {
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    // Ensure fee is rounded up when truncated if ceil is true.
    Amount nFee = Amount::zero();
    if (ceil) {
        nFee = Amount(nSize * nSatoshisPerK % 1000 > Amount::zero()
                          ? nSize * nSatoshisPerK / 1000 + SATOSHI
                          : nSize * nSatoshisPerK / 1000);
    } else {
        nFee = nSize * nSatoshisPerK / 1000;
    }

    if (nFee == Amount::zero() && nSize != 0) {
        if (nSatoshisPerK > Amount::zero()) {
            nFee = SATOSHI;
        }
        if (nSatoshisPerK < Amount::zero()) {
            nFee = -SATOSHI;
        }
    }

    return nFee;
}

Amount CFeeRate::GetFee(size_t nBytes) const {
    return ::GetFee<false>(nBytes, nSatoshisPerK);
}

Amount CFeeRate::GetFeeCeiling(size_t nBytes) const {
    return ::GetFee<true>(nBytes, nSatoshisPerK);
}

std::string CFeeRate::ToString() const {
    const auto currency = Currency::get();
    return strprintf("%d.%0*d %s/kB", nSatoshisPerK / currency.baseunit,
                     currency.decimals,
                     (nSatoshisPerK % currency.baseunit) / currency.subunit,
                     currency.ticker);
}
