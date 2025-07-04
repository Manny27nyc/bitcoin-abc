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
#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of getpeerinfo RPC banscore field."""

from test_framework.test_framework import BitcoinTestFramework


class GetpeerinfoBanscoreDeprecationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], ["-deprecatedrpc=banscore"]]

    def run_test(self):
        self.log.info(
            "Test getpeerinfo by default no longer returns a banscore field")
        assert "banscore" not in self.nodes[0].getpeerinfo()[0].keys()

        self.log.info(
            "Test getpeerinfo returns banscore with -deprecatedrpc=banscore")
        assert "banscore" in self.nodes[1].getpeerinfo()[0].keys()


if __name__ == "__main__":
    GetpeerinfoBanscoreDeprecationTest().main()
