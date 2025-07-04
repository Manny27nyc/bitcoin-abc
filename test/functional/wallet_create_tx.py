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
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
)


class CreateTxWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('Create some old blocks')
        self.nodes[0].setmocktime(TIME_GENESIS_BLOCK)
        self.nodes[0].generate(200)
        self.nodes[0].setmocktime(0)

        self.test_anti_fee_sniping()
        self.test_tx_size_too_large()

    def test_anti_fee_sniping(self):
        self.log.info(
            'Check that we have some (old) blocks and that anti-fee-sniping is disabled')
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], 200)
        txid = self.nodes[0].sendtoaddress(
            self.nodes[0].getnewaddress(), 1000000)
        tx = self.nodes[0].decoderawtransaction(
            self.nodes[0].gettransaction(txid)['hex'])
        assert_equal(tx['locktime'], 0)

        self.log.info(
            'Check that anti-fee-sniping is enabled when we mine a recent block')
        self.nodes[0].generate(1)
        txid = self.nodes[0].sendtoaddress(
            self.nodes[0].getnewaddress(), 1000000)
        tx = self.nodes[0].decoderawtransaction(
            self.nodes[0].gettransaction(txid)['hex'])
        assert 0 < tx['locktime'] <= 201

    def test_tx_size_too_large(self):
        # More than 10kB of outputs, so that we hit -maxtxfee with a high
        # feerate
        outputs = {self.nodes[0].getnewaddress(): 25 for i in range(400)}
        raw_tx = self.nodes[0].createrawtransaction(inputs=[], outputs=outputs)

        for fee_setting in ['-minrelaytxfee=10000',
                            '-mintxfee=10000', '-paytxfee=10000']:
            self.log.info(
                'Check maxtxfee in combination with {}'.format(fee_setting))
            self.restart_node(0, extra_args=[fee_setting])
            assert_raises_rpc_error(
                -6,
                "Fee exceeds maximum configured by -maxtxfee",
                lambda: self.nodes[0].sendmany(dummy="", amounts=outputs),
            )
            assert_raises_rpc_error(
                -4,
                "Fee exceeds maximum configured by -maxtxfee",
                lambda: self.nodes[0].fundrawtransaction(hexstring=raw_tx),
            )

        self.log.info('Check maxtxfee in combination with settxfee')
        self.restart_node(0)
        self.nodes[0].settxfee(10000)
        assert_raises_rpc_error(
            -6,
            "Fee exceeds maximum configured by -maxtxfee",
            lambda: self.nodes[0].sendmany(dummy="", amounts=outputs),
        )
        assert_raises_rpc_error(
            -4,
            "Fee exceeds maximum configured by -maxtxfee",
            lambda: self.nodes[0].fundrawtransaction(hexstring=raw_tx),
        )
        self.nodes[0].settxfee(0)


if __name__ == '__main__':
    CreateTxWalletTest().main()
