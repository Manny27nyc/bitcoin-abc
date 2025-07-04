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
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coinbase transactions return the correct categories.

Tests listtransactions, listsinceblock, and gettransaction.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_array_result
)


class CoinbaseCategoryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_category(self, category, address, txid, skip):
        assert_array_result(self.nodes[0].listtransactions(skip=skip),
                            {"address": address},
                            {"category": category})
        assert_array_result(self.nodes[0].listsinceblock()["transactions"],
                            {"address": address},
                            {"category": category})
        assert_array_result(self.nodes[0].gettransaction(txid)["details"],
                            {"address": address},
                            {"category": category})

    def run_test(self):
        # Generate one block to an address
        address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(1, address)
        hash = self.nodes[0].getbestblockhash()
        txid = self.nodes[0].getblock(hash)["tx"][0]

        # Coinbase transaction is immature after 1 confirmation
        self.assert_category("immature", address, txid, 0)

        # Mine another 99 blocks on top
        self.nodes[0].generate(99)
        # Coinbase transaction is still immature after 100 confirmations
        self.assert_category("immature", address, txid, 99)

        # Mine one more block
        self.nodes[0].generate(1)
        # Coinbase transaction is now matured, so category is "generate"
        self.assert_category("generate", address, txid, 100)

        # Orphan block that paid to address
        self.nodes[0].invalidateblock(hash)
        # Coinbase transaction is now orphaned
        self.assert_category("orphan", address, txid, 100)


if __name__ == '__main__':
    CoinbaseCategoryTest().main()
