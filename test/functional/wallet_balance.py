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
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet balance RPC methods."""
from decimal import Decimal
import struct

from test_framework.address import ADDRESS_BCHREG_UNSPENDABLE as ADDRESS_WATCHONLY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    connect_nodes,
)


def create_transactions(node, address, amt, fees):
    # Create and sign raw transactions from node to address for amt.
    # Creates a transaction for each fee and returns an array
    # of the raw transactions.
    utxos = [u for u in node.listunspent(0) if u['spendable']]

    # Create transactions
    inputs = []
    ins_total = 0
    for utxo in utxos:
        inputs.append({"txid": utxo["txid"], "vout": utxo["vout"]})
        ins_total += utxo['amount']
        if ins_total >= amt + max(fees):
            break
    # make sure there was enough utxos
    assert ins_total >= amt + max(fees)

    txs = []
    for fee in fees:
        outputs = {address: amt}
        # prevent 0 change output
        if ins_total > amt + fee:
            outputs[node.getrawchangeaddress()] = ins_total - amt - fee
        raw_tx = node.createrawtransaction(inputs, outputs, 0)
        raw_tx = node.signrawtransactionwithwallet(raw_tx)
        assert_equal(raw_tx['complete'], True)
        txs.append(raw_tx)

    return txs


class WalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [
            # Limit mempool descendants as a hack to have wallet txs rejected
            # from the mempool
            ['-limitdescendantcount=3'],
            [],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].importaddress(ADDRESS_WATCHONLY)
        # Check that nodes don't own any UTXOs
        assert_equal(len(self.nodes[0].listunspent()), 0)
        assert_equal(len(self.nodes[1].listunspent()), 0)

        self.log.info("Check that only node 0 is watching an address")
        assert 'watchonly' in self.nodes[0].getbalances()
        assert 'watchonly' not in self.nodes[1].getbalances()

        self.log.info("Mining blocks ...")
        self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[1].generate(1)
        self.nodes[1].generatetoaddress(101, ADDRESS_WATCHONLY)
        self.sync_all()

        assert_equal(self.nodes[0].getbalances()['mine']['trusted'], 50000000)
        assert_equal(self.nodes[0].getwalletinfo()['balance'], 50000000)
        assert_equal(self.nodes[1].getbalances()['mine']['trusted'], 50000000)

        assert_equal(self.nodes[0].getbalances()[
                     'watchonly']['immature'], 5000000000)
        assert 'watchonly' not in self.nodes[1].getbalances()

        assert_equal(self.nodes[0].getbalance(), 50000000)
        assert_equal(self.nodes[1].getbalance(), 50000000)

        self.log.info("Test getbalance with different arguments")
        assert_equal(self.nodes[0].getbalance("*"), 50000000)
        assert_equal(self.nodes[0].getbalance("*", 1), 50000000)
        assert_equal(self.nodes[0].getbalance("*", 1, True), 100000000)
        assert_equal(self.nodes[0].getbalance(minconf=1), 50000000)
        assert_equal(
            self.nodes[0].getbalance(
                minconf=0,
                include_watchonly=True),
            100000000)
        assert_equal(
            self.nodes[1].getbalance(
                minconf=0,
                include_watchonly=True),
            50000000)

        # Send 40 BTC from 0 to 1 and 60 BTC from 1 to 0.
        txs = create_transactions(
            self.nodes[0], self.nodes[1].getnewaddress(), 40000000, [Decimal('10000')])
        self.nodes[0].sendrawtransaction(txs[0]['hex'])
        # sending on both nodes is faster than waiting for propagation
        self.nodes[1].sendrawtransaction(txs[0]['hex'])

        self.sync_all()
        txs = create_transactions(self.nodes[1], self.nodes[0].getnewaddress(), 60000000, [
                                  Decimal('10000'), Decimal('20000')])
        self.nodes[1].sendrawtransaction(txs[0]['hex'])
        # sending on both nodes is faster than waiting for propagation
        self.nodes[0].sendrawtransaction(txs[0]['hex'])
        self.sync_all()

        # First argument of getbalance must be set to "*"
        assert_raises_rpc_error(-32, "dummy first argument must be excluded or set to \"*\"",
                                self.nodes[1].getbalance, "")

        self.log.info("Test balances with unconfirmed inputs")

        # Before `test_balance()`, we have had two nodes with a balance of 50
        # each and then we:
        #
        # 1) Sent 40 from node A to node B with fee 0.01
        # 2) Sent 60 from node B to node A with fee 0.01
        #
        # Then we check the balances:
        #
        # 1) As is
        # 2) With transaction 2 from above with 2x the fee
        #
        # Prior to #16766, in this situation, the node would immediately report
        # a balance of 30 on node B as unconfirmed and trusted.
        #
        # After #16766, we show that balance as unconfirmed.
        #
        # The balance is indeed "trusted" and "confirmed" insofar as removing
        # the mempool transactions would return at least that much money. But
        # the algorithm after #16766 marks it as unconfirmed because the 'taint'
        # tracking of transaction trust for summing balances doesn't consider
        # which inputs belong to a user. In this case, the change output in
        # question could be "destroyed" by replace the 1st transaction above.
        #
        # The post #16766 behavior is correct; we shouldn't be treating those
        # funds as confirmed. If you want to rely on that specific UTXO existing
        # which has given you that balance, you cannot, as a third party
        # spending the other input would destroy that unconfirmed.
        #
        # For example, if the test transactions were:
        #
        # 1) Sent 40 from node A to node B with fee 0.01
        # 2) Sent 10 from node B to node A with fee 0.01
        #
        # Then our node would report a confirmed balance of 40 + 50 - 10 = 80
        # BTC, which is more than would be available if transaction 1 were
        # replaced.

        def test_balances(*, fee_node_1=0):
            # getbalances
            expected_balances_0 = {'mine': {'immature': Decimal('0E-2'),
                                            # change from node 0's send
                                            'trusted': Decimal('9990000'),
                                            'untrusted_pending': Decimal('60000000.0')},
                                   'watchonly': {'immature': Decimal('5000000000'),
                                                 'trusted': Decimal('50000000.0'),
                                                 'untrusted_pending': Decimal('0E-2')}}
            expected_balances_1 = {'mine': {'immature': Decimal('0E-2'),
                                            # node 1's send had an unsafe input
                                            'trusted': Decimal('0E-2'),
                                            # Doesn't include output of node
                                            # 0's send since it was spent
                                            'untrusted_pending': Decimal('30000000.0') - fee_node_1}}
            assert_equal(self.nodes[0].getbalances(), expected_balances_0)
            assert_equal(self.nodes[1].getbalances(), expected_balances_1)
            # getbalance without any arguments includes unconfirmed transactions, but not untrusted transactions
            # change from node 0's send
            assert_equal(self.nodes[0].getbalance(), Decimal('9990000'))
            # node 1's send had an unsafe input
            assert_equal(self.nodes[1].getbalance(), Decimal('0'))
            # Same with minconf=0
            assert_equal(
                self.nodes[0].getbalance(
                    minconf=0),
                Decimal('9990000'))
            assert_equal(self.nodes[1].getbalance(minconf=0), Decimal('0'))
            # getbalance with a minconf incorrectly excludes coins that have been spent more recently than the minconf blocks ago
            # TODO: fix getbalance tracking of coin spentness depth
            assert_equal(self.nodes[0].getbalance(minconf=1), Decimal('0'))
            assert_equal(self.nodes[1].getbalance(minconf=1), Decimal('0'))
            # getunconfirmedbalance
            # output of node 1's spend
            assert_equal(
                self.nodes[0].getunconfirmedbalance(),
                Decimal('60000000'))

            # Doesn't include output of node 0's send since it was spent
            assert_equal(
                self.nodes[1].getunconfirmedbalance(),
                Decimal('30000000') - fee_node_1)
            # getwalletinfo.unconfirmed_balance
            assert_equal(self.nodes[0].getwalletinfo()[
                         "unconfirmed_balance"], Decimal('60000000'))
            assert_equal(
                self.nodes[1].getwalletinfo()["unconfirmed_balance"],
                Decimal('30000000') - fee_node_1)

        test_balances(fee_node_1=Decimal('10000'))

        # In the original Core version of this test, Node 1 would've bumped
        # the fee by 0.01 here to resend, but this is BCH, so it has 0.01 BCH
        # left to spend on goods and services
        self.sync_all()

        self.log.info(
            "Test getbalance and getbalances.mine.untrusted_pending with conflicted unconfirmed inputs")
        test_balances(fee_node_1=Decimal('10000'))

        self.nodes[1].generatetoaddress(1, ADDRESS_WATCHONLY)
        self.sync_all()

        # balances are correct after the transactions are confirmed
        # node 1's send plus change from node 0's send
        balance_node0 = Decimal('69990000')
        # change from node 0's send
        balance_node1 = Decimal('29990000')
        assert_equal(self.nodes[0].getbalances()[
                     'mine']['trusted'], balance_node0)
        assert_equal(self.nodes[1].getbalances()[
                     'mine']['trusted'], balance_node1)
        assert_equal(self.nodes[0].getbalance(), balance_node0)
        assert_equal(self.nodes[1].getbalance(), balance_node1)

        # Send total balance away from node 1
        txs = create_transactions(self.nodes[1], self.nodes[0].getnewaddress(
        ), Decimal('29970000'), [Decimal('10000')])
        self.nodes[1].sendrawtransaction(txs[0]['hex'])
        self.nodes[1].generatetoaddress(2, ADDRESS_WATCHONLY)
        self.sync_all()

        # getbalance with a minconf incorrectly excludes coins that have been spent more recently than the minconf blocks ago
        # TODO: fix getbalance tracking of coin spentness depth
        # getbalance with minconf=3 should still show the old balance
        assert_equal(self.nodes[1].getbalance(minconf=3), Decimal('0'))

        # getbalance with minconf=2 will show the new balance.
        assert_equal(self.nodes[1].getbalance(minconf=2), Decimal('10000'))

        # check mempool transactions count for wallet unconfirmed balance after
        # dynamically loading the wallet.
        before = self.nodes[1].getbalances()['mine']['untrusted_pending']
        dst = self.nodes[1].getnewaddress()
        self.nodes[1].unloadwallet('')
        self.nodes[0].sendtoaddress(dst, 100000)
        self.sync_all()
        self.nodes[1].loadwallet('')
        after = self.nodes[1].getbalances()['mine']['untrusted_pending']
        assert_equal(before + Decimal('100000'), after)

        # Create 3 more wallet txs, where the last is not accepted to the
        # mempool because it is the third descendant of the tx above
        for _ in range(3):
            # Set amount high enough such that all coins are spent by each tx
            txid = self.nodes[0].sendtoaddress(
                self.nodes[0].getnewaddress(), 99000000)

        self.log.info('Check that wallet txs not in the mempool are untrusted')
        assert txid not in self.nodes[0].getrawmempool()
        assert_equal(self.nodes[0].gettransaction(txid)['trusted'], False)
        assert_equal(self.nodes[0].getbalance(minconf=0), 0)

        self.log.info("Test replacement and reorg of non-mempool tx")
        tx_orig = self.nodes[0].gettransaction(txid)['hex']
        # Increase fee by 1 coin
        tx_replace = tx_orig.replace(
            struct.pack("<q", 99 * 10**8).hex(),
            struct.pack("<q", 98 * 10**8).hex(),
        )
        tx_replace = self.nodes[0].signrawtransactionwithwallet(tx_replace)[
            'hex']
        # Total balance is given by the sum of outputs of the tx
        total_amount = sum(
            [o['value'] for o in self.nodes[0].decoderawtransaction(tx_replace)['vout']])
        self.sync_all()
        self.nodes[1].sendrawtransaction(hexstring=tx_replace, maxfeerate=0)

        # Now confirm tx_replace
        block_reorg = self.nodes[1].generatetoaddress(1, ADDRESS_WATCHONLY)[0]
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(minconf=0), total_amount)

        self.log.info('Put txs back into mempool of node 1 (not node 0)')
        self.nodes[0].invalidateblock(block_reorg)
        self.nodes[1].invalidateblock(block_reorg)
        self.sync_blocks()
        self.nodes[0].syncwithvalidationinterfacequeue()
        # wallet txs not in the mempool are untrusted
        assert_equal(self.nodes[0].getbalance(minconf=0), 0)
        self.nodes[0].generatetoaddress(1, ADDRESS_WATCHONLY)
        # wallet txs not in the mempool are untrusted
        assert_equal(self.nodes[0].getbalance(minconf=0), 0)

        # Now confirm tx_orig
        self.restart_node(1, ['-persistmempool=0'])
        connect_nodes(self.nodes[0], self.nodes[1])
        self.sync_blocks()
        self.nodes[1].sendrawtransaction(tx_orig)
        self.nodes[1].generatetoaddress(1, ADDRESS_WATCHONLY)
        self.sync_all()
        # The reorg recovered our fee of 1 coin
        assert_equal(
            self.nodes[0].getbalance(
                minconf=0),
            total_amount +
            1000000)


if __name__ == '__main__':
    WalletTest().main()
