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
"""Test requesting invalid blocks behaves safely."""

from test_framework.messages import (
    CInv,
    msg_getblocks,
    msg_getdata,
    msg_getheaders,
    MSG_BLOCK,
    MSG_CMPCT_BLOCK,
)
from test_framework.p2p import p2p_lock, P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class GetInvalidBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        node.add_p2p_connection(P2PInterface())
        chaintip = node.getbestblockhash()

        # Mine some blocks and invalidate them
        blocks = node.generatetoaddress(
            3, node.get_deterministic_priv_key().address)
        assert_equal(blocks[-1], node.getbestblockhash())
        node.invalidateblock(blocks[0])
        assert_equal(chaintip, node.getbestblockhash())

        # Clear any old messages
        with p2p_lock:
            node.p2p.last_message.pop("block", None)
            node.p2p.last_message.pop("cmpctblock", None)
            node.p2p.last_message.pop("headers", None)

        # Requests for the invalidated block and it's decendants should fail.
        # Not doing so is a potential DoS vector.
        for b in blocks:
            block_hash = int(b, 16)

            # Currently, the implementation for getblocks skips blocks which
            # are not on the currently active chain. This is the only logged
            # indication of such.
            with node.assert_debug_log(expected_msgs=["getblocks -1 to"]):
                msg = msg_getblocks()
                msg.locator.vHave = [block_hash]
                node.p2p.send_message(msg)
                node.p2p.sync_with_ping()

            with node.assert_debug_log(expected_msgs=["ignoring request from peer=0 for old block that isn't in the main chain"]):
                msg = msg_getdata()
                msg.inv.append(CInv(MSG_BLOCK, block_hash))
                node.p2p.send_message(msg)
                node.p2p.sync_with_ping()

            with node.assert_debug_log(expected_msgs=["ignoring request from peer=0 for old block that isn't in the main chain"]):
                msg = msg_getdata()
                msg.inv.append(CInv(MSG_CMPCT_BLOCK, block_hash))
                node.p2p.send_message(msg)
                node.p2p.sync_with_ping()

            with node.assert_debug_log(expected_msgs=["ignoring request from peer=0 for old block header that isn't in the main chain"]):
                msg = msg_getheaders()
                msg.hashstop = block_hash
                node.p2p.send_message(msg)
                node.p2p.sync_with_ping()


if __name__ == '__main__':
    GetInvalidBlockTest().main()
