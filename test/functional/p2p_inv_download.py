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
"""
Test inventory download behavior
"""

from test_framework.address import ADDRESS_BCHREG_UNSPENDABLE
from test_framework.avatools import wait_for_proof
from test_framework.key import ECKey, bytes_to_wif
from test_framework.messages import (
    AvalancheProof,
    CInv,
    CTransaction,
    FromHex,
    MSG_AVA_PROOF,
    MSG_TX,
    MSG_TYPE_MASK,
    msg_avaproof,
    msg_inv,
    msg_notfound,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    wait_until,
)

import functools
import time


class TestP2PConn(P2PInterface):
    def __init__(self, inv_type):
        super().__init__()
        self.inv_type = inv_type
        self.getdata_count = 0

    def on_getdata(self, message):
        for i in message.inv:
            if i.type & MSG_TYPE_MASK == self.inv_type:
                self.getdata_count += 1


class NetConstants:
    """Constants from net_processing"""

    def __init__(self,
                 getdata_interval,
                 inbound_peer_delay,
                 overloaded_peer_delay,
                 max_getdata_in_flight,
                 max_peer_announcements,
                 bypass_request_limits_permission_flags,
                 ):
        self.getdata_interval = getdata_interval
        self.inbound_peer_delay = inbound_peer_delay
        self.overloaded_peer_delay = overloaded_peer_delay
        self.max_getdata_in_flight = max_getdata_in_flight
        self.max_peer_announcements = max_peer_announcements
        self.max_getdata_inbound_wait = self.getdata_interval + self.inbound_peer_delay
        self.bypass_request_limits_permission_flags = bypass_request_limits_permission_flags


class TestContext:
    def __init__(self, inv_type, inv_name, constants):
        self.inv_type = inv_type
        self.inv_name = inv_name
        self.constants = constants

    def p2p_conn(self):
        return TestP2PConn(self.inv_type)


PROOF_TEST_CONTEXT = TestContext(
    MSG_AVA_PROOF,
    "avalanche proof",
    NetConstants(
        getdata_interval=60,  # seconds
        inbound_peer_delay=2,  # seconds
        overloaded_peer_delay=2,  # seconds
        max_getdata_in_flight=100,
        max_peer_announcements=5000,
        bypass_request_limits_permission_flags="bypass_proof_request_limits",
    ),
)

TX_TEST_CONTEXT = TestContext(
    MSG_TX,
    "transaction",
    NetConstants(
        getdata_interval=60,  # seconds
        inbound_peer_delay=2,  # seconds
        overloaded_peer_delay=2,  # seconds
        max_getdata_in_flight=100,
        max_peer_announcements=5000,
        bypass_request_limits_permission_flags="relay",
    ),
)

# Python test constants
NUM_INBOUND = 10


def skip(context):
    def decorator(test):
        @functools.wraps(test)
        def wrapper(*args, **kwargs):
            # Assume the signature is test(self, context) unless context is
            # passed by name
            call_context = kwargs.get("context", args[1])
            if call_context == context:
                return lambda *args, **kwargs: None
            return test(*args, **kwargs)
        return wrapper
    return decorator


class InventoryDownloadTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 2
        self.extra_args = [['-enableavalanche=1',
                            '-avacooldown=0']] * self.num_nodes

    def test_data_requests(self, context):
        self.log.info(
            "Test that we request data from all our peers, eventually")

        invid = 0xdeadbeef

        self.log.info("Announce the invid from each incoming peer to node 0")
        msg = msg_inv([CInv(t=context.inv_type, h=invid)])
        for p in self.nodes[0].p2ps:
            p.send_and_ping(msg)

        outstanding_peer_index = [i for i in range(len(self.nodes[0].p2ps))]

        def getdata_found(peer_index):
            p = self.nodes[0].p2ps[peer_index]
            with p2p_lock:
                return p.last_message.get(
                    "getdata") and p.last_message["getdata"].inv[-1].hash == invid

        node_0_mocktime = int(time.time())
        while outstanding_peer_index:
            node_0_mocktime += context.constants.max_getdata_inbound_wait
            self.nodes[0].setmocktime(node_0_mocktime)
            wait_until(lambda: any(getdata_found(i)
                                   for i in outstanding_peer_index))
            for i in outstanding_peer_index:
                if getdata_found(i):
                    outstanding_peer_index.remove(i)

        self.nodes[0].setmocktime(0)
        self.log.info("All outstanding peers received a getdata")

    @skip(PROOF_TEST_CONTEXT)
    def test_inv_block(self, context):
        self.log.info("Generate a transaction on node 0")
        tx = self.nodes[0].createrawtransaction(
            inputs=[{
                # coinbase
                "txid": self.nodes[0].getblock(self.nodes[0].getblockhash(1))['tx'][0],
                "vout": 0
            }],
            outputs={ADDRESS_BCHREG_UNSPENDABLE: 50000000 - 250.00},
        )
        tx = self.nodes[0].signrawtransactionwithkey(
            hexstring=tx,
            privkeys=[self.nodes[0].get_deterministic_priv_key().key],
        )['hex']
        ctx = FromHex(CTransaction(), tx)
        txid = int(ctx.rehash(), 16)

        self.log.info(
            "Announce the transaction to all nodes from all {} incoming peers, but never send it".format(NUM_INBOUND))
        msg = msg_inv([CInv(t=context.inv_type, h=txid)])
        for p in self.peers:
            p.send_and_ping(msg)

        self.log.info("Put the tx in node 0's mempool")
        self.nodes[0].sendrawtransaction(tx)

        # Since node 1 is connected outbound to an honest peer (node 0), it
        # should get the tx within a timeout. (Assuming that node 0
        # announced the tx within the timeout)
        # The timeout is the sum of
        # * the worst case until the tx is first requested from an inbound
        #   peer, plus
        # * the first time it is re-requested from the outbound peer, plus
        # * 2 seconds to avoid races
        assert self.nodes[1].getpeerinfo()[0]['inbound'] is False
        timeout = 2 + context.constants.inbound_peer_delay + \
            context.constants.getdata_interval
        self.log.info(
            "Tx should be received at node 1 after {} seconds".format(timeout))
        self.sync_mempools(timeout=timeout)

    def test_in_flight_max(self, context):
        max_getdata_in_flight = context.constants.max_getdata_in_flight
        max_inbound_delay = context.constants.inbound_peer_delay + \
            context.constants.overloaded_peer_delay

        self.log.info("Test that we don't load peers with more than {} getdata requests immediately".format(
            max_getdata_in_flight))
        invids = [i for i in range(max_getdata_in_flight + 2)]

        p = self.nodes[0].p2ps[0]

        with p2p_lock:
            p.getdata_count = 0

        mock_time = int(time.time() + 1)
        self.nodes[0].setmocktime(mock_time)
        for i in range(max_getdata_in_flight):
            p.send_message(msg_inv([CInv(t=context.inv_type, h=invids[i])]))
        p.sync_with_ping()
        mock_time += context.constants.inbound_peer_delay
        self.nodes[0].setmocktime(mock_time)
        p.wait_until(lambda: p.getdata_count >= max_getdata_in_flight)
        for i in range(max_getdata_in_flight, len(invids)):
            p.send_message(msg_inv([CInv(t=context.inv_type, h=invids[i])]))
        p.sync_with_ping()
        self.log.info(
            "No more than {} requests should be seen within {} seconds after announcement".format(
                max_getdata_in_flight,
                max_inbound_delay - 1))
        self.nodes[0].setmocktime(
            mock_time +
            max_inbound_delay - 1)
        p.sync_with_ping()
        with p2p_lock:
            assert_equal(p.getdata_count, max_getdata_in_flight)
        self.log.info(
            "If we wait {} seconds after announcement, we should eventually get more requests".format(
                max_inbound_delay))
        self.nodes[0].setmocktime(
            mock_time +
            max_inbound_delay)
        p.wait_until(lambda: p.getdata_count == len(invids))

    def test_expiry_fallback(self, context):
        self.log.info(
            'Check that expiry will select another peer for download')
        peer1 = self.nodes[0].add_p2p_connection(context.p2p_conn())
        peer2 = self.nodes[0].add_p2p_connection(context.p2p_conn())
        for p in [peer1, peer2]:
            p.send_message(msg_inv([CInv(t=context.inv_type, h=0xffaa)]))
        # One of the peers is asked for the data
        peer2.wait_until(
            lambda: sum(
                p.getdata_count for p in [
                    peer1, peer2]) == 1)
        with p2p_lock:
            peer_expiry, peer_fallback = (
                peer1, peer2) if peer1.getdata_count == 1 else (
                peer2, peer1)
            assert_equal(peer_fallback.getdata_count, 0)
        # Wait for request to peer_expiry to expire
        self.nodes[0].setmocktime(
            int(time.time()) + context.constants.getdata_interval + 1)
        peer_fallback.wait_until(
            lambda: peer_fallback.getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer_fallback.getdata_count, 1)
        # reset mocktime
        self.restart_node(0)

    def test_disconnect_fallback(self, context):
        self.log.info(
            'Check that disconnect will select another peer for download')
        peer1 = self.nodes[0].add_p2p_connection(context.p2p_conn())
        peer2 = self.nodes[0].add_p2p_connection(context.p2p_conn())
        for p in [peer1, peer2]:
            p.send_message(msg_inv([CInv(t=context.inv_type, h=0xffbb)]))
        # One of the peers is asked for the data
        peer2.wait_until(
            lambda: sum(
                p.getdata_count for p in [
                    peer1, peer2]) == 1)
        with p2p_lock:
            peer_disconnect, peer_fallback = (
                peer1, peer2) if peer1.getdata_count == 1 else (
                peer2, peer1)
            assert_equal(peer_fallback.getdata_count, 0)
        peer_disconnect.peer_disconnect()
        peer_disconnect.wait_for_disconnect()
        peer_fallback.wait_until(
            lambda: peer_fallback.getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer_fallback.getdata_count, 1)

    def test_notfound_fallback(self, context):
        self.log.info(
            'Check that notfounds will select another peer for download immediately')
        peer1 = self.nodes[0].add_p2p_connection(context.p2p_conn())
        peer2 = self.nodes[0].add_p2p_connection(context.p2p_conn())
        for p in [peer1, peer2]:
            p.send_message(msg_inv([CInv(t=context.inv_type, h=0xffdd)]))
        # One of the peers is asked for the data
        peer2.wait_until(
            lambda: sum(
                p.getdata_count for p in [
                    peer1, peer2]) == 1)
        with p2p_lock:
            peer_notfound, peer_fallback = (
                peer1, peer2) if peer1.getdata_count == 1 else (
                peer2, peer1)
            assert_equal(peer_fallback.getdata_count, 0)
        # Send notfound, so that fallback peer is selected
        peer_notfound.send_and_ping(msg_notfound(
            vec=[CInv(context.inv_type, 0xffdd)]))
        peer_fallback.wait_until(
            lambda: peer_fallback.getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer_fallback.getdata_count, 1)

    def test_preferred_inv(self, context):
        self.log.info(
            'Check that invs from preferred peers are downloaded immediately')
        self.restart_node(
            0,
            extra_args=self.extra_args[0] +
            ['-whitelist=noban@127.0.0.1'])
        peer = self.nodes[0].add_p2p_connection(context.p2p_conn())
        peer.send_message(msg_inv([CInv(t=context.inv_type, h=0xff00ff00)]))
        peer.wait_until(lambda: peer.getdata_count >= 1, timeout=1)
        with p2p_lock:
            assert_equal(peer.getdata_count, 1)

    def test_large_inv_batch(self, context):
        max_peer_announcements = context.constants.max_peer_announcements
        net_permissions = context.constants.bypass_request_limits_permission_flags
        self.log.info(
            'Test how large inv batches are handled with {} permission'.format(net_permissions))
        self.restart_node(
            0,
            extra_args=self.extra_args[0] +
            ['-whitelist={}@127.0.0.1'.format(net_permissions)])
        peer = self.nodes[0].add_p2p_connection(context.p2p_conn())
        peer.send_message(msg_inv([CInv(t=context.inv_type, h=invid)
                                   for invid in range(max_peer_announcements + 1)]))
        peer.wait_until(lambda: peer.getdata_count ==
                        max_peer_announcements + 1)

        self.log.info(
            'Test how large inv batches are handled without {} permission'.format(net_permissions))
        self.restart_node(0)
        peer = self.nodes[0].add_p2p_connection(context.p2p_conn())
        peer.send_message(msg_inv([CInv(t=context.inv_type, h=invid)
                                   for invid in range(max_peer_announcements + 1)]))
        peer.wait_until(lambda: peer.getdata_count ==
                        max_peer_announcements)
        peer.sync_with_ping()
        with p2p_lock:
            assert_equal(peer.getdata_count, max_peer_announcements)

    def test_spurious_notfound(self, context):
        self.log.info('Check that spurious notfound is ignored')
        self.nodes[0].p2ps[0].send_message(
            msg_notfound(vec=[CInv(context.inv_type, 1)]))

    @skip(TX_TEST_CONTEXT)
    def test_orphan_download(self, context):
        node = self.nodes[0]
        privkey = ECKey()
        privkey.generate()
        privkey_wif = bytes_to_wif(privkey.get_bytes())

        # Build a proof with missing utxos so it will be orphaned
        orphan = node.buildavalancheproof(
            42, 2000000000, privkey.get_pubkey().get_bytes().hex(), [{
                'txid': '0' * 64,
                'vout': 0,
                'amount': 10e6,
                'height': 42,
                'iscoinbase': False,
                'privatekey': privkey_wif,
            }]
        )
        proofid = FromHex(AvalancheProof(), orphan).proofid
        proofid_hex = "{:064x}".format(proofid)

        self.restart_node(0, extra_args=self.extra_args[0] + [
            "-avaproof={}".format(orphan),
            "-avamasterkey={}".format(privkey_wif),
        ])
        node.generate(1)
        wait_for_proof(node, proofid_hex, expect_orphan=True)

        peer = node.add_p2p_connection(context.p2p_conn())
        peer.send_message(msg_inv([CInv(t=context.inv_type, h=proofid)]))

        # Give enough time for the node to eventually request the proof.
        node.setmocktime(int(time.time()) +
                         context.constants.getdata_interval + 1)
        peer.sync_with_ping()

        assert_equal(peer.getdata_count, 0)

    @skip(TX_TEST_CONTEXT)
    def test_request_invalid_once(self, context):
        node = self.nodes[0]
        privkey = ECKey()
        privkey.generate()

        # Build an invalid proof (no stake)
        no_stake_hex = node.buildavalancheproof(
            42, 2000000000, privkey.get_pubkey().get_bytes().hex(), []
        )
        no_stake = FromHex(AvalancheProof(), no_stake_hex)
        assert_raises_rpc_error(-8,
                                "The proof is invalid: no-stake",
                                node.verifyavalancheproof,
                                no_stake_hex)

        # Send the proof
        msg = msg_avaproof()
        msg.proof = no_stake
        node.p2ps[0].send_message(msg)

        # Check we get banned
        node.p2ps[0].wait_for_disconnect()

        # Now that the node knows the proof is invalid, it should not be
        # requested anymore
        node.p2ps[1].send_message(
            msg_inv([CInv(t=context.inv_type, h=no_stake.proofid)]))

        # Give enough time for the node to eventually request the proof
        node.setmocktime(int(time.time()) +
                         context.constants.getdata_interval + 1)
        node.p2ps[1].sync_with_ping()

        assert all(p.getdata_count == 0 for p in node.p2ps[1:])

    def run_test(self):
        for context in [TX_TEST_CONTEXT, PROOF_TEST_CONTEXT]:
            self.log.info(
                "Starting tests using " +
                context.inv_name +
                " inventory type")

            # Run tests without mocktime that only need one peer-connection first,
            # to avoid restarting the nodes
            self.test_expiry_fallback(context)
            self.test_disconnect_fallback(context)
            self.test_notfound_fallback(context)
            self.test_preferred_inv(context)
            self.test_large_inv_batch(context)
            self.test_spurious_notfound(context)

            # Run each test against new bitcoind instances, as setting mocktimes has long-term effects on when
            # the next trickle relay event happens.
            for test in [self.test_in_flight_max, self.test_inv_block,
                         self.test_data_requests, self.test_orphan_download, self.test_request_invalid_once]:
                self.stop_nodes()
                self.start_nodes()
                self.connect_nodes(1, 0)
                # Setup the p2p connections
                self.peers = []
                for node in self.nodes:
                    for _ in range(NUM_INBOUND):
                        self.peers.append(
                            node.add_p2p_connection(
                                context.p2p_conn()))
                self.log.info(
                    "Nodes are setup with {} incoming connections each".format(NUM_INBOUND))
                test(context)


if __name__ == '__main__':
    InventoryDownloadTest().main()
