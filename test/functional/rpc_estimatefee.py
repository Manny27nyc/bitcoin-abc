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
# Copyright (c) 2019 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class EstimateFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            [],
            ["-minrelaytxfee=1000"],
            ["-mintxfee=20", "-maxtxfee=25"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        default_node = self.nodes[0]
        diff_relay_fee_node = self.nodes[1]
        diff_tx_fee_node = self.nodes[2]
        for i in range(5):
            self.nodes[0].generate(1)

            # estimatefee is 0.00001 by default, regardless of block contents
            assert_equal(default_node.estimatefee(), Decimal('10.00'))

            # estimatefee may be different for nodes that set it in their
            # config
            assert_equal(diff_relay_fee_node.estimatefee(), Decimal('1000.00'))

            # Check the reasonableness of settxfee
            assert_raises_rpc_error(-8, "txfee cannot be less than min relay tx fee",
                                    diff_tx_fee_node.settxfee, Decimal('5.00'))
            assert_raises_rpc_error(-8, "txfee cannot be less than wallet min fee",
                                    diff_tx_fee_node.settxfee, Decimal('15.00'))
            assert_raises_rpc_error(-8, "txfee cannot be more than wallet max tx fee",
                                    diff_tx_fee_node.settxfee, Decimal('30.00'))


if __name__ == '__main__':
    EstimateFeeTest().main()
