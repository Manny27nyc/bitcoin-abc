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
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getchaintips RPC.

- introduce a network split
- work on chains of different lengths
- join the network together again
- verify that getchaintips now returns two chain tips.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class GetChainTipsTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.extra_args = [["-noparkdeepreorg"], ["-noparkdeepreorg"], [], []]

    def run_test(self):
        tips = self.nodes[0].getchaintips()
        assert_equal(len(tips), 1)
        assert_equal(tips[0]['branchlen'], 0)
        assert_equal(tips[0]['height'], 200)
        assert_equal(tips[0]['status'], 'active')

        # Split the network and build two chains of different lengths.
        self.split_network()
        self.nodes[0].generatetoaddress(
            10, self.nodes[0].get_deterministic_priv_key().address)
        self.nodes[2].generatetoaddress(
            20, self.nodes[2].get_deterministic_priv_key().address)
        self.sync_all(self.nodes[:2])
        self.sync_all(self.nodes[2:])

        tips = self.nodes[1].getchaintips()
        assert_equal(len(tips), 1)
        shortTip = tips[0]
        assert_equal(shortTip['branchlen'], 0)
        assert_equal(shortTip['height'], 210)
        assert_equal(tips[0]['status'], 'active')

        tips = self.nodes[3].getchaintips()
        assert_equal(len(tips), 1)
        longTip = tips[0]
        assert_equal(longTip['branchlen'], 0)
        assert_equal(longTip['height'], 220)
        assert_equal(tips[0]['status'], 'active')

        # Join the network halves and check that we now have two tips
        # (at least at the nodes that previously had the short chain).
        self.join_network()

        tips = self.nodes[0].getchaintips()
        assert_equal(len(tips), 2)
        assert_equal(tips[0], longTip)

        assert_equal(tips[1]['branchlen'], 10)
        assert_equal(tips[1]['status'], 'valid-fork')
        tips[1]['branchlen'] = 0
        tips[1]['status'] = 'active'
        assert_equal(tips[1], shortTip)


if __name__ == '__main__':
    GetChainTipsTest().main()
