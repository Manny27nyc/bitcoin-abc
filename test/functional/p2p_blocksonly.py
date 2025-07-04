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
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p blocksonly"""

from test_framework.messages import msg_tx, CTransaction, FromHex
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class P2PBlocksOnly(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [["-blocksonly"]]

    def run_test(self):
        self.nodes[0].add_p2p_connection(P2PInterface())

        self.log.info(
            'Check that txs from p2p are rejected and result in disconnect')
        prevtx = self.nodes[0].getblock(
            self.nodes[0].getblockhash(1), 2)['tx'][0]
        rawtx = self.nodes[0].createrawtransaction(
            inputs=[{
                'txid': prevtx['txid'],
                'vout': 0
            }],
            outputs=[{
                self.nodes[0].get_deterministic_priv_key().address: 50000000 -
                1250.00
            }],
        )
        self.log.info(prevtx)
        sigtx = self.nodes[0].signrawtransactionwithkey(
            hexstring=rawtx,
            privkeys=[self.nodes[0].get_deterministic_priv_key().key],
            prevtxs=[{
                'txid': prevtx['txid'],
                'vout': 0,
                'amount': prevtx['vout'][0]['value'],
                'scriptPubKey': prevtx['vout'][0]['scriptPubKey']['hex'],
            }],
        )['hex']
        assert_equal(self.nodes[0].getnetworkinfo()['localrelay'], False)
        with self.nodes[0].assert_debug_log(['transaction sent in violation of protocol peer=0']):
            self.nodes[0].p2p.send_message(
                msg_tx(FromHex(CTransaction(), sigtx)))
            self.nodes[0].p2p.wait_for_disconnect()
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 0)

        # Remove the disconnected peer and add a new one.
        del self.nodes[0].p2ps[0]
        self.nodes[0].add_p2p_connection(P2PInterface())

        self.log.info(
            'Check that txs from rpc are not rejected and relayed to other peers')
        assert_equal(self.nodes[0].getpeerinfo()[0]['relaytxes'], True)
        txid = self.nodes[0].testmempoolaccept([sigtx])[0]['txid']
        with self.nodes[0].assert_debug_log(['received getdata for: tx {} peer=1'.format(txid)]):
            self.nodes[0].sendrawtransaction(sigtx)
            self.nodes[0].p2p.wait_for_tx(txid)
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 1)

        self.log.info(
            'Check that txs from whitelisted peers are not rejected and relayed to others')
        self.log.info(
            "Restarting node 0 with whitelist permission and blocksonly")
        self.restart_node(0,
                          ["-persistmempool=0",
                           "-whitelist=127.0.0.1",
                           "-whitelistforcerelay",
                           "-blocksonly"])
        assert_equal(self.nodes[0].getrawmempool(), [])
        first_peer = self.nodes[0].add_p2p_connection(P2PInterface())
        second_peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer_1_info = self.nodes[0].getpeerinfo()[0]
        assert_equal(peer_1_info['whitelisted'], True)
        assert_equal(
            peer_1_info['permissions'], [
                'noban', 'forcerelay', 'relay', 'mempool', 'download'])
        peer_2_info = self.nodes[0].getpeerinfo()[1]
        assert_equal(peer_2_info['whitelisted'], True)
        assert_equal(
            peer_2_info['permissions'], [
                'noban', 'forcerelay', 'relay', 'mempool', 'download'])
        assert_equal(
            self.nodes[0].testmempoolaccept(
                [sigtx])[0]['allowed'], True)
        txid = self.nodes[0].testmempoolaccept([sigtx])[0]['txid']

        self.log.info(
            'Check that the tx from whitelisted first_peer is relayed to others (ie.second_peer)')
        with self.nodes[0].assert_debug_log(["received getdata"]):
            first_peer.send_message(msg_tx(FromHex(CTransaction(), sigtx)))
            self.log.info(
                'Check that the whitelisted peer is still connected after sending the transaction')
            assert_equal(first_peer.is_connected, True)
            second_peer.wait_for_tx(txid)
            assert_equal(self.nodes[0].getmempoolinfo()['size'], 1)
        self.log.info("Whitelisted peer's transaction is accepted and relayed")


if __name__ == '__main__':
    P2PBlocksOnly().main()
