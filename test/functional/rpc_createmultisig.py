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
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multisig RPCs"""

from test_framework.descriptors import descsum_create, drop_origins
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
)
from test_framework.key import ECPubKey

import binascii
import decimal
import itertools
import json
import os


class RpcCreateMultiSigTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_keys(self):
        node0, node1, node2 = self.nodes
        add = [node1.getnewaddress() for _ in range(self.nkeys)]
        self.pub = [node1.getaddressinfo(a)["pubkey"] for a in add]
        self.priv = [node1.dumpprivkey(a) for a in add]
        self.final = node2.getnewaddress()

    def run_test(self):
        node0, node1, node2 = self.nodes

        self.check_addmultisigaddress_errors()

        self.log.info('Generating blocks ...')
        node0.generate(149)
        self.sync_all()

        self.moved = 0
        for self.nkeys in [3, 5]:
            for self.nsigs in [2, 3]:
                self.get_keys()
                self.do_multisig()

        self.checkbalances()

    # Test mixed compressed and uncompressed pubkeys
        self.log.info(
            'Mixed compressed and uncompressed multisigs are not allowed')
        pk0 = node0.getaddressinfo(node0.getnewaddress())['pubkey']
        pk1 = node1.getaddressinfo(node1.getnewaddress())['pubkey']
        pk2 = node2.getaddressinfo(node2.getnewaddress())['pubkey']

        # decompress pk2
        pk_obj = ECPubKey()
        pk_obj.set(binascii.unhexlify(pk2))
        pk_obj.compressed = False
        pk2 = binascii.hexlify(pk_obj.get_bytes()).decode()

        # Check all permutations of keys because order matters apparently
        for keys in itertools.permutations([pk0, pk1, pk2]):
            # Results should be the same as this legacy one
            legacy_addr = node0.createmultisig(2, keys)['address']
            assert_equal(
                legacy_addr, node0.addmultisigaddress(
                    2, keys, '')['address'])

            # Generate addresses with the segwit types. These should all make
            # legacy addresses
            assert_equal(legacy_addr, node0.createmultisig(2, keys)['address'])

        self.log.info(
            'Testing sortedmulti descriptors with BIP 67 test vectors')
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/rpc_bip67.json'), encoding='utf-8') as f:
            vectors = json.load(f)

        for t in vectors:
            key_str = ','.join(t['keys'])
            desc = descsum_create('sh(sortedmulti(2,{}))'.format(key_str))
            assert_equal(self.nodes[0].deriveaddresses(desc)[0], t['address'])
            sorted_key_str = ','.join(t['sorted_keys'])
            sorted_key_desc = descsum_create(
                'sh(multi(2,{}))'.format(sorted_key_str))
            assert_equal(self.nodes[0].deriveaddresses(
                sorted_key_desc)[0], t['address'])

    def check_addmultisigaddress_errors(self):
        self.log.info(
            'Check that addmultisigaddress fails when the private keys are missing')
        addresses = [self.nodes[1].getnewaddress(
            address_type='legacy') for _ in range(2)]
        assert_raises_rpc_error(-5,
                                'no full public key for address',
                                lambda: self.nodes[0].addmultisigaddress(nrequired=1,
                                                                         keys=addresses))
        for a in addresses:
            # Importing all addresses should not change the result
            self.nodes[0].importaddress(a)
        assert_raises_rpc_error(-5,
                                'no full public key for address',
                                lambda: self.nodes[0].addmultisigaddress(nrequired=1,
                                                                         keys=addresses))

    def checkbalances(self):
        node0, node1, node2 = self.nodes
        node0.generate(100)
        self.sync_all()

        bal0 = node0.getbalance()
        bal1 = node1.getbalance()
        bal2 = node2.getbalance()

        height = node0.getblockchaininfo()["blocks"]
        assert 150 < height < 350
        total = 149 * 50000000 + (height - 149 - 100) * 25000000
        assert bal1 == 0
        assert bal2 == self.moved
        assert bal0 + bal1 + bal2 == total

    def do_multisig(self):
        node0, node1, node2 = self.nodes

        # Construct the expected descriptor
        desc = 'multi({},{})'.format(self.nsigs, ','.join(self.pub))
        desc = 'sh({})'.format(desc)
        desc = descsum_create(desc)

        msig = node2.createmultisig(self.nsigs, self.pub)
        madd = msig["address"]
        mredeem = msig["redeemScript"]
        assert_equal(desc, msig['descriptor'])

        # compare against addmultisigaddress
        msigw = node1.addmultisigaddress(self.nsigs, self.pub, None)
        maddw = msigw["address"]
        mredeemw = msigw["redeemScript"]
        assert_equal(desc, drop_origins(msigw['descriptor']))
        # addmultisigiaddress and createmultisig work the same
        assert maddw == madd
        assert mredeemw == mredeem

        txid = node0.sendtoaddress(madd, 40000000)

        tx = node0.getrawtransaction(txid, True)
        vout = [v["n"] for v in tx["vout"]
                if madd in v["scriptPubKey"].get("addresses", [])]
        assert len(vout) == 1
        vout = vout[0]
        scriptPubKey = tx["vout"][vout]["scriptPubKey"]["hex"]
        value = tx["vout"][vout]["value"]
        prevtxs = [{"txid": txid, "vout": vout, "scriptPubKey": scriptPubKey,
                    "redeemScript": mredeem, "amount": value}]

        node0.generate(1)

        outval = value - decimal.Decimal("10.00")
        rawtx = node2.createrawtransaction(
            [{"txid": txid, "vout": vout}], [{self.final: outval}])

        rawtx2 = node2.signrawtransactionwithkey(
            rawtx, self.priv[0:self.nsigs - 1], prevtxs)
        rawtx3 = node2.signrawtransactionwithkey(
            rawtx2["hex"], [self.priv[-1]], prevtxs)

        self.moved += outval
        tx = node0.sendrawtransaction(rawtx3["hex"], 0)
        blk = node0.generate(1)[0]
        assert tx in node0.getblock(blk)["tx"]

        txinfo = node0.getrawtransaction(tx, True, blk)
        self.log.info("n/m={}/{} size={}".format(self.nsigs,
                                                 self.nkeys, txinfo["size"]))


if __name__ == '__main__':
    RpcCreateMultiSigTest().main()
