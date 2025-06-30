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
# Copyright (c) 2020-2021 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the peer discovery behavior of avalanche nodes.

This includes tests for the service flag, avahello handshake and
proof exchange.
"""

import time

from test_framework.avatools import (
    get_ava_p2p_interface,
    create_coinbase_stakes,
    get_proof_ids,
)
from test_framework.key import (
    bytes_to_wif,
    ECKey,
    ECPubKey,
)
from test_framework.p2p import p2p_lock
from test_framework.messages import (
    AvalancheProof,
    CInv,
    FromHex,
    MSG_AVA_PROOF,
    msg_getdata,
    NODE_AVALANCHE,
    NODE_NETWORK,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wait_until,
)

UNCONDITIONAL_RELAY_DELAY = 2 * 60


class AvalancheTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-enableavalanche=1']]
        self.supports_cli = False

    def run_test(self):
        node = self.nodes[0]

        # duplicate the deterministic sig test from src/test/key_tests.cpp
        privkey = ECKey()
        privkey.set(bytes.fromhex(
            "12b004fff7f4b69ef8650e767f18f11ede158148b425660723b9f9a66e61f747"), True)
        pubkey = privkey.get_pubkey()

        self.log.info(
            "Check the node is signalling the avalanche service bit only if there is a proof.")
        assert_equal(
            int(node.getnetworkinfo()['localservices'], 16) & NODE_AVALANCHE,
            0)

        # Create stakes by mining blocks
        addrkey0 = node.get_deterministic_priv_key()
        blockhashes = node.generatetoaddress(2, addrkey0.address)
        stakes = create_coinbase_stakes(node, [blockhashes[0]], addrkey0.key)

        proof_sequence = 11
        proof_expiration = 12
        proof = node.buildavalancheproof(
            proof_sequence, proof_expiration, pubkey.get_bytes().hex(),
            stakes)

        # Restart the node
        self.restart_node(0, self.extra_args[0] + [
            "-avaproof={}".format(proof),
            "-avamasterkey=cND2ZvtabDbJ1gucx9GWH6XT9kgTAqfb6cotPt5Q5CyxVDhid2EN",
        ])

        assert_equal(
            int(node.getnetworkinfo()['localservices'], 16) & NODE_AVALANCHE,
            NODE_AVALANCHE)

        def check_avahello(args):
            # Restart the node with the given args
            self.restart_node(0, self.extra_args[0] + args)

            peer = get_ava_p2p_interface(node)

            avahello = peer.wait_for_avahello().hello

            avakey = ECPubKey()
            avakey.set(bytes.fromhex(node.getavalanchekey()))
            assert avakey.verify_schnorr(
                avahello.sig, avahello.get_sighash(peer))

        self.log.info(
            "Test the avahello signature with a generated delegation")
        check_avahello([
            "-avaproof={}".format(proof),
            "-avamasterkey=cND2ZvtabDbJ1gucx9GWH6XT9kgTAqfb6cotPt5Q5CyxVDhid2EN"
        ])

        master_key = ECKey()
        master_key.generate()
        limited_id = FromHex(AvalancheProof(), proof).limited_proofid
        delegation = node.delegateavalancheproof(
            f"{limited_id:0{64}x}",
            bytes_to_wif(privkey.get_bytes()),
            master_key.get_pubkey().get_bytes().hex(),
        )

        self.log.info("Test the avahello signature with a supplied delegation")
        check_avahello([
            "-avaproof={}".format(proof),
            "-avadelegation={}".format(delegation),
            "-avamasterkey={}".format(bytes_to_wif(master_key.get_bytes())),
        ])

        stakes = create_coinbase_stakes(node, [blockhashes[1]], addrkey0.key)
        interface_proof_hex = node.buildavalancheproof(
            proof_sequence, proof_expiration, pubkey.get_bytes().hex(),
            stakes)
        limited_id = FromHex(
            AvalancheProof(),
            interface_proof_hex).limited_proofid

        # delegate
        delegated_key = ECKey()
        delegated_key.generate()
        interface_delegation_hex = node.delegateavalancheproof(
            f"{limited_id:0{64}x}",
            bytes_to_wif(privkey.get_bytes()),
            delegated_key.get_pubkey().get_bytes().hex(),
            None)

        self.log.info("Test that wrong avahello signature causes a ban")
        bad_interface = get_ava_p2p_interface(node)
        wrong_key = ECKey()
        wrong_key.generate()
        with node.assert_debug_log(
                ["Misbehaving",
                 "peer=1 (0 -> 100) BAN THRESHOLD EXCEEDED: invalid-avahello-signature"]):
            bad_interface.send_avahello(interface_delegation_hex, wrong_key)
            bad_interface.wait_for_disconnect()

        self.log.info(
            'Check that receiving a valid avahello triggers a proof getdata request')
        good_interface = get_ava_p2p_interface(node)
        proofid = good_interface.send_avahello(
            interface_delegation_hex, delegated_key)

        def getdata_found():
            with p2p_lock:
                return good_interface.last_message.get(
                    "getdata") and good_interface.last_message["getdata"].inv[-1].hash == proofid
        wait_until(getdata_found)

        self.log.info('Check that we can download the proof from our peer')

        node_proofid = FromHex(AvalancheProof(), proof).proofid

        def wait_for_proof_validation():
            # Connect some blocks to trigger the proof verification
            node.generate(1)
            wait_until(lambda: node_proofid in get_proof_ids(node))

        wait_for_proof_validation()

        getdata = msg_getdata([CInv(MSG_AVA_PROOF, node_proofid)])

        self.log.info(
            "Proof has been inv'ed recently, check it can be requested")
        good_interface.send_message(getdata)

        def proof_received(peer):
            with p2p_lock:
                return peer.last_message.get(
                    "avaproof") and peer.last_message["avaproof"].proof.proofid == node_proofid
        wait_until(lambda: proof_received(good_interface))

        # Restart the node
        self.restart_node(0, self.extra_args[0] + [
            "-avaproof={}".format(proof),
            "-avamasterkey=cND2ZvtabDbJ1gucx9GWH6XT9kgTAqfb6cotPt5Q5CyxVDhid2EN",
        ])
        wait_for_proof_validation()

        self.log.info(
            "The proof has not been announced, it cannot be requested")
        peer = get_ava_p2p_interface(node, services=NODE_NETWORK)
        peer.send_message(getdata)

        # Give enough time for the node to answer. Since we cannot check for a
        # non-event this is the best we can do
        time.sleep(2)
        assert not proof_received(peer)

        self.log.info("The proof is known for long enough to be requested")
        current_time = int(time.time())
        node.setmocktime(current_time + UNCONDITIONAL_RELAY_DELAY)

        peer.send_message(getdata)
        wait_until(lambda: proof_received(peer))


if __name__ == '__main__':
    AvalancheTest().main()
