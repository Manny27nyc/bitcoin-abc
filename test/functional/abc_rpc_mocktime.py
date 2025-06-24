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
# Copyright (c) 2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test RPCs related to mock time.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class MocktimeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.nodes[0].setmocktime(9223372036854775807)
        self.nodes[0].setmocktime(0)
        assert_raises_rpc_error(-8, "Timestamp must be 0 or greater",
                                self.nodes[0].setmocktime, -1)
        assert_raises_rpc_error(-8, "Timestamp must be 0 or greater",
                                self.nodes[0].setmocktime, -9223372036854775808)


if __name__ == '__main__':
    MocktimeTest().main()
