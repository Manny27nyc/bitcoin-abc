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
"""Test the abandontransaction RPC.

 The abandontransaction RPC marks a transaction and all its in-wallet
 descendants as abandoned which allows their inputs to be respent. It can be
 used to replace "stuck" or evicted transactions. It only works on transactions
 which are not included in a block and are not currently in the mempool. It has
 no effect on transactions which are already abandoned.
"""
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    connect_nodes,
    disconnect_nodes,
    satoshi_round,
)


class AbandonConflictTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-minrelaytxfee=10"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        def total_fees(*txids):
            total = 0
            for txid in txids:
                # '-=' is because gettransaction(txid)['fee'] returns a negative
                total -= self.nodes[0].gettransaction(txid)['fee']
            return satoshi_round(total)

        self.nodes[1].generate(100)
        self.sync_blocks()
        balance = self.nodes[0].getbalance()
        txA = self.nodes[0].sendtoaddress(
            self.nodes[0].getnewaddress(), Decimal("10000000"))
        txB = self.nodes[0].sendtoaddress(
            self.nodes[0].getnewaddress(), Decimal("10000000"))
        txC = self.nodes[0].sendtoaddress(
            self.nodes[0].getnewaddress(), Decimal("10000000"))

        self.sync_mempools()
        self.nodes[1].generate(1)

        # Can not abandon non-wallet transaction
        assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id',
                                lambda: self.nodes[0].abandontransaction(txid='ff' * 32))
        # Can not abandon confirmed transaction
        assert_raises_rpc_error(-5, 'Transaction not eligible for abandonment',
                                lambda: self.nodes[0].abandontransaction(txid=txA))

        self.sync_blocks()
        newbalance = self.nodes[0].getbalance()

        # no more than fees lost
        assert balance - newbalance <= total_fees(txA, txB, txC)
        balance = newbalance

        # Disconnect nodes so node0's transactions don't get into node1's
        # mempool
        disconnect_nodes(self.nodes[0], self.nodes[1])

        # Identify the 10btc outputs
        nA = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(
            txA)["details"] if tx_out["amount"] == Decimal("10000000"))
        nB = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(
            txB)["details"] if tx_out["amount"] == Decimal("10000000"))
        nC = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(
            txC)["details"] if tx_out["amount"] == Decimal("10000000"))

        inputs = []
        # spend 10btc outputs from txA and txB
        inputs.append({"txid": txA, "vout": nA})
        inputs.append({"txid": txB, "vout": nB})
        outputs = {}

        outputs[self.nodes[0].getnewaddress()] = Decimal("14999980")
        outputs[self.nodes[1].getnewaddress()] = Decimal("5000000")
        signed = self.nodes[0].signrawtransactionwithwallet(
            self.nodes[0].createrawtransaction(inputs, outputs))
        txAB1 = self.nodes[0].sendrawtransaction(signed["hex"])

        # Identify the 14.99998btc output
        nAB = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(
            txAB1)["details"] if tx_out["amount"] == Decimal("14999980"))

        # Create a child tx spending AB1 and C
        inputs = []
        # Amount 14.99998 BCH
        inputs.append({"txid": txAB1, "vout": nAB})
        # Amount 10 BCH
        inputs.append({"txid": txC, "vout": nC})
        outputs = {}
        outputs[self.nodes[0].getnewaddress()] = Decimal("24999600")
        signed2 = self.nodes[0].signrawtransactionwithwallet(
            self.nodes[0].createrawtransaction(inputs, outputs))
        txABC2 = self.nodes[0].sendrawtransaction(signed2["hex"])

        # Create a child tx spending ABC2
        signed3_change = Decimal("24999000")
        inputs = [{"txid": txABC2, "vout": 0}]
        outputs = {self.nodes[0].getnewaddress(): signed3_change}
        signed3 = self.nodes[0].signrawtransactionwithwallet(
            self.nodes[0].createrawtransaction(inputs, outputs))
        # note tx is never directly referenced, only abandoned as a child of
        # the above
        self.nodes[0].sendrawtransaction(signed3["hex"])

        # In mempool txs from self should increase balance from change
        newbalance = self.nodes[0].getbalance()
        assert_equal(
            newbalance,
            balance -
            Decimal("30000000") +
            signed3_change)
        balance = newbalance

        # Restart the node with a higher min relay fee so the parent tx is no longer in mempool
        # TODO: redo with eviction
        self.restart_node(0, extra_args=["-minrelaytxfee=100"])
        assert self.nodes[0].getmempoolinfo()['loaded']

        # Verify txs no longer in either node's mempool
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        # Transactions which are not in the mempool should only reduce wallet balance.
        # Transaction inputs should still be spent, but the change not yet
        # received.
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - signed3_change)
        # Unconfirmed received funds that are not in mempool also shouldn't show
        # up in unconfirmed balance.  Note that the transactions stored in the wallet
        # are not necessarily in the node's mempool.
        balances = self.nodes[0].getbalances()['mine']
        assert_equal(
            balances['untrusted_pending'] +
            balances['trusted'],
            newbalance)
        # Unconfirmed transactions which are not in the mempool should also
        # not be in listunspent
        assert txABC2 not in [utxo["txid"]
                              for utxo in self.nodes[0].listunspent(0)]
        balance = newbalance

        # Abandon original transaction and verify inputs are available again
        # including that the child tx was also abandoned
        self.nodes[0].abandontransaction(txAB1)
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance + Decimal("30000000"))
        balance = newbalance

        # Verify that even with a low min relay fee, the tx is not re-accepted
        # from wallet on startup once abandoned.
        self.restart_node(0, extra_args=["-minrelaytxfee=10"])
        assert self.nodes[0].getmempoolinfo()['loaded']

        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(self.nodes[0].getbalance(), balance)

        # If the transaction is re-sent the wallet also unabandons it. The
        # change should be available, and it's child transaction should remain
        # abandoned.
        # NOTE: Abandoned transactions are internal to the wallet, and tracked
        # separately from other indices.
        self.nodes[0].sendrawtransaction(signed["hex"])
        newbalance = self.nodes[0].getbalance()
        assert_equal(
            newbalance,
            balance -
            Decimal("20000000") +
            Decimal("14999980"))
        balance = newbalance

        # Send child tx again so it is no longer abandoned.
        self.nodes[0].sendrawtransaction(signed2["hex"])
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - Decimal("10000000")
                     - Decimal("14999980") + Decimal("24999600"))
        balance = newbalance

        # Reset to a higher relay fee so that we abandon a transaction
        self.restart_node(0, extra_args=["-minrelaytxfee=100"])
        assert self.nodes[0].getmempoolinfo()['loaded']
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - Decimal("24999600"))
        balance = newbalance

        # Create a double spend of AB1. Spend it again from only A's 10 output.
        # Mine double spend from node 1.
        inputs = []
        inputs.append({"txid": txA, "vout": nA})
        outputs = {}
        outputs[self.nodes[1].getnewaddress()] = Decimal("9999900")
        tx = self.nodes[0].createrawtransaction(inputs, outputs)
        signed = self.nodes[0].signrawtransactionwithwallet(tx)
        self.nodes[1].sendrawtransaction(signed["hex"])
        self.nodes[1].generate(1)

        connect_nodes(self.nodes[0], self.nodes[1])
        self.sync_blocks()

        # Verify that B and C's 10 BCH outputs are available for spending again
        # because AB1 is now conflicted
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance + Decimal("20000000"))
        balance = newbalance

        # There is currently a minor bug around this and so this test doesn't work.  See Issue #7315
        # Invalidate the block with the double spend and B's 10 BCH output should no longer be available
        # Don't think C's should either
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        newbalance = self.nodes[0].getbalance()
        # assert_equal(newbalance, balance - Decimal("10"))
        self.log.info(
            "If balance has not declined after invalidateblock then out of mempool wallet tx which is no longer")
        self.log.info(
            "conflicted has not resumed causing its inputs to be seen as spent.  See Issue #7315")
        self.log.info(str(balance) + " -> " + str(newbalance) + " ?")


if __name__ == '__main__':
    AbandonConflictTest().main()
