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

from decimal import Decimal

from test_framework.messages import CTransaction, FromHex
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    connect_nodes,
    find_vout_for_address,
)


def get_unspent(listunspent, amount):
    for utx in listunspent:
        if utx['amount'] == amount:
            return utx
    raise AssertionError(
        'Could not find unspent with amount={}'.format(amount))


class RawTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        # This test isn't testing tx relay. Set whitelist on the peers for
        # instant tx relay.
        self.extra_args = [['-whitelist=noban@127.0.0.1']] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

        connect_nodes(self.nodes[0], self.nodes[1])
        connect_nodes(self.nodes[1], self.nodes[2])
        connect_nodes(self.nodes[0], self.nodes[2])
        connect_nodes(self.nodes[0], self.nodes[3])

    def run_test(self):
        self.log.info("Connect nodes, set fees, generate blocks, and sync")
        self.min_relay_tx_fee = self.nodes[0].getnetworkinfo()['relayfee']
        # This test is not meant to test fee estimation and we'd like
        # to be sure all txs are sent at a consistent desired feerate
        for node in self.nodes:
            node.settxfee(self.min_relay_tx_fee)

        # if the fee's positive delta is higher than this value tests will fail,
        # neg. delta always fail the tests.
        # The size of the signature of every input may be at most 2 bytes larger
        # than a minimum sized signature.

        #            = 2 bytes * minRelayTxFeePerByte
        self.fee_tolerance = 2 * self.min_relay_tx_fee / 1000

        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(121)
        self.sync_all()

        self.test_change_position()
        self.test_simple()
        self.test_simple_two_coins()
        self.test_simple_two_outputs()
        self.test_change()
        self.test_no_change()
        self.test_invalid_option()
        self.test_invalid_change_address()
        self.test_valid_change_address()
        self.test_coin_selection()
        self.test_two_vin()
        self.test_two_vin_two_vout()
        self.test_invalid_input()
        self.test_fee_p2pkh()
        self.test_fee_p2pkh_multi_out()
        self.test_fee_p2sh()
        self.test_fee_4of5()
        self.test_spend_2of2()
        self.test_locked_wallet()
        self.test_many_inputs_fee()
        self.test_many_inputs_send()
        self.test_op_return()
        self.test_watchonly()
        self.test_all_watched_funds()
        self.test_option_feerate()
        self.test_address_reuse()
        self.test_option_subtract_fee_from_outputs()
        self.test_subtract_fee_with_presets()

    def test_change_position(self):
        """Ensure setting changePosition in fundraw with an exact match is
        handled properly."""
        self.log.info("Test fundrawtxn changePosition option")
        rawmatch = self.nodes[2].createrawtransaction(
            [], {self.nodes[2].getnewaddress(): 50000000})
        rawmatch = self.nodes[2].fundrawtransaction(
            rawmatch, {"changePosition": 1, "subtractFeeFromOutputs": [0]})
        assert_equal(rawmatch["changepos"], -1)

        watchonly_address = self.nodes[0].getnewaddress()
        watchonly_pubkey = self.nodes[0].getaddressinfo(watchonly_address)[
            "pubkey"]
        self.watchonly_amount = Decimal(200000000)
        self.nodes[3].importpubkey(watchonly_pubkey, "", True)
        self.watchonly_txid = self.nodes[0].sendtoaddress(
            watchonly_address, self.watchonly_amount)

        # Lock UTXO so nodes[0] doesn't accidentally spend it
        self.watchonly_vout = find_vout_for_address(
            self.nodes[0], self.watchonly_txid, watchonly_address)
        self.nodes[0].lockunspent(
            False, [{"txid": self.watchonly_txid, "vout": self.watchonly_vout}])

        self.nodes[0].sendtoaddress(
            self.nodes[3].getnewaddress(),
            self.watchonly_amount / 10)

        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1500000)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1000000)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 5000000)

        self.nodes[0].generate(1)
        self.sync_all()

    def test_simple(self):
        self.log.info("Test fundrawtxn")
        inputs = []
        outputs = {self.nodes[0].getnewaddress(): 1000000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        # test that we have enough inputs
        assert len(dec_tx['vin']) > 0

    def test_simple_two_coins(self):
        self.log.info("Test fundrawtxn with 2 coins")
        inputs = []
        outputs = {self.nodes[0].getnewaddress(): 2200000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        # test if we have enough inputs
        assert len(dec_tx['vin']) > 0
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')

    def test_simple_two_outputs(self):
        self.log.info("Test fundrawtxn with 2 outputs")
        inputs = []
        outputs = {
            self.nodes[0].getnewaddress(): 2600000, self.nodes[1].getnewaddress(): 2500000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        assert len(dec_tx['vin']) > 0
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')

    def test_change(self):
        self.log.info("Test fundrawtxn with a vin > required amount")
        utx = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']}]
        outputs = {self.nodes[0].getnewaddress(): 1000000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)
        fee = rawtxfund['fee']
        # Use the same fee for the next tx
        self.test_no_change_fee = fee
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        # compare vin total and totalout+fee
        assert_equal(fee + totalOut, utx['amount'])

    def test_no_change(self):
        self.log.info("Test fundrawtxn not having a change output")
        utx = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']}]
        outputs = {
            self.nodes[0].getnewaddress(): Decimal(5000000) -
            self.test_no_change_fee -
            self.fee_tolerance}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)
        fee = rawtxfund['fee']
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        assert_equal(rawtxfund['changepos'], -1)
        # compare vin total and totalout+fee
        assert_equal(fee + totalOut, utx['amount'])

    def test_invalid_option(self):
        self.log.info("Test fundrawtxn with an invalid option")
        utx = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']}]
        outputs = {self.nodes[0].getnewaddress(): Decimal(4000000)}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(-3, "Unexpected key foo", self.nodes[
            2].fundrawtransaction, rawTx, {'foo': 'bar'})
        # reserveChangeKey was deprecated and is now removed
        assert_raises_rpc_error(-3,
                                "Unexpected key reserveChangeKey",
                                lambda: self.nodes[2].fundrawtransaction(hexstring=rawTx,
                                                                         options={'reserveChangeKey': True}))

    def test_invalid_change_address(self):
        self.log.info("Test fundrawtxn with an invalid change address")
        utx = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']}]
        outputs = {self.nodes[0].getnewaddress(): Decimal(4000000)}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(
            -5, "changeAddress must be a valid bitcoin address",
            self.nodes[2].fundrawtransaction, rawTx, {'changeAddress': 'foobar'})

    def test_valid_change_address(self):
        self.log.info("Test fundrawtxn with a provided change address")
        utx = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']}]
        outputs = {self.nodes[0].getnewaddress(): Decimal(4000000)}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        change = self.nodes[2].getnewaddress()
        assert_raises_rpc_error(-8, "changePosition out of bounds", self.nodes[
            2].fundrawtransaction, rawTx, {'changeAddress': change, 'changePosition': 2})
        rawtxfund = self.nodes[2].fundrawtransaction(
            rawTx, {'changeAddress': change, 'changePosition': 0})
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        out = dec_tx['vout'][0]
        assert_equal(change, out['scriptPubKey']['addresses'][0])

    def test_coin_selection(self):
        self.log.info("Test fundrawtxn with a vin < required amount")
        utx = get_unspent(self.nodes[2].listunspent(), 1000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']}]
        outputs = {self.nodes[0].getnewaddress(): 1000000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)

        # 4-byte version + 1-byte vin count + 36-byte prevout then script_len
        rawTx = rawTx[:82] + "0100" + rawTx[84:]

        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])
        assert_equal("00", dec_tx['vin'][0]['scriptSig']['hex'])

        # Should fail without add_inputs:
        assert_raises_rpc_error(-4,
                                "Insufficient funds",
                                self.nodes[2].fundrawtransaction,
                                rawTx,
                                {"add_inputs": False})
        # add_inputs is enabled by default
        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)

        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for i, out in enumerate(dec_tx['vout']):
            totalOut += out['value']
            if out['scriptPubKey']['addresses'][0] in outputs:
                matchingOuts += 1
            else:
                assert_equal(i, rawtxfund['changepos'])

        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])
        assert_equal("00", dec_tx['vin'][0]['scriptSig']['hex'])

        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)

    def test_two_vin(self):
        self.log.info("Test fundrawtxn with 2 vins")
        utx = get_unspent(self.nodes[2].listunspent(), 1000000)
        utx2 = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']},
                  {'txid': utx2['txid'], 'vout': utx2['vout']}]
        outputs = {self.nodes[0].getnewaddress(): 6000000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        # Should fail without add_inputs:
        assert_raises_rpc_error(-4,
                                "Insufficient funds",
                                self.nodes[2].fundrawtransaction,
                                rawTx,
                                {"add_inputs": False})
        rawtxfund = self.nodes[2].fundrawtransaction(
            rawTx, {"add_inputs": True})

        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if out['scriptPubKey']['addresses'][0] in outputs:
                matchingOuts += 1

        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)

        matchingIns = 0
        for vinOut in dec_tx['vin']:
            for vinIn in inputs:
                if vinIn['txid'] == vinOut['txid']:
                    matchingIns += 1

        # we now must see two vins identical to vins given as params
        assert_equal(matchingIns, 2)

    def test_two_vin_two_vout(self):
        self.log.info("Test fundrawtxn with 2 vins and 2 vouts")
        utx = get_unspent(self.nodes[2].listunspent(), 1000000)
        utx2 = get_unspent(self.nodes[2].listunspent(), 5000000)

        inputs = [{'txid': utx['txid'], 'vout': utx['vout']},
                  {'txid': utx2['txid'], 'vout': utx2['vout']}]
        outputs = {
            self.nodes[0].getnewaddress(): 6000000, self.nodes[0].getnewaddress(): 1000000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        # Should fail without add_inputs:
        assert_raises_rpc_error(-4,
                                "Insufficient funds",
                                self.nodes[2].fundrawtransaction,
                                rawTx,
                                {"add_inputs": False})
        rawtxfund = self.nodes[2].fundrawtransaction(
            rawTx, {"add_inputs": True})

        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if out['scriptPubKey']['addresses'][0] in outputs:
                matchingOuts += 1

        assert_equal(matchingOuts, 2)
        assert_equal(len(dec_tx['vout']), 3)

    def test_invalid_input(self):
        self.log.info("Test fundrawtxn with an invalid vin")
        inputs = [
            {'txid': "1c7f966dab21119bac53213a2bc7532bff1fa844c124fd750a7d0b1332440bd1", 'vout': 0}]
        outputs = {self.nodes[0].getnewaddress(): 1000000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)

        assert_raises_rpc_error(
            -4, "Insufficient funds", self.nodes[2].fundrawtransaction, rawTx)

    def test_fee_p2pkh(self):
        """Compare fee of a standard pubkeyhash transaction."""
        self.log.info("Test fundrawtxn p2pkh fee")
        inputs = []
        outputs = {self.nodes[1].getnewaddress(): 1100000}
        rawTx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawTx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendtoaddress(
            self.nodes[1].getnewaddress(), 1100000)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

    def test_fee_p2pkh_multi_out(self):
        """Compare fee of a standard pubkeyhash transaction with multiple
        outputs."""
        self.log.info("Test fundrawtxn p2pkh fee with multiple outputs")
        inputs = []
        outputs = {
            self.nodes[1].getnewaddress(): 1100000,
            self.nodes[1].getnewaddress(): 1200000,
            self.nodes[1].getnewaddress(): 100000,
            self.nodes[1].getnewaddress(): 1300000,
            self.nodes[1].getnewaddress(): 200000,
            self.nodes[1].getnewaddress(): 300000,
        }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)
        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendmany("", outputs)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

    def test_fee_p2sh(self):
        """Compare fee of a 2-of-2 multisig p2sh transaction."""
        # Create 2-of-2 addr.
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[1].getnewaddress()

        addr1Obj = self.nodes[1].getaddressinfo(addr1)
        addr2Obj = self.nodes[1].getaddressinfo(addr2)

        mSigObj = self.nodes[1].addmultisigaddress(
            2, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']

        inputs = []
        outputs = {mSigObj: 1100000}
        rawTx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawTx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendtoaddress(mSigObj, 1100000)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

    def test_fee_4of5(self):
        """Compare fee of a standard pubkeyhash transaction."""
        self.log.info("Test fundrawtxn fee with 4-of-5 addresses")

        # Create 4-of-5 addr.
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[1].getnewaddress()
        addr3 = self.nodes[1].getnewaddress()
        addr4 = self.nodes[1].getnewaddress()
        addr5 = self.nodes[1].getnewaddress()

        addr1Obj = self.nodes[1].getaddressinfo(addr1)
        addr2Obj = self.nodes[1].getaddressinfo(addr2)
        addr3Obj = self.nodes[1].getaddressinfo(addr3)
        addr4Obj = self.nodes[1].getaddressinfo(addr4)
        addr5Obj = self.nodes[1].getaddressinfo(addr5)

        mSigObj = self.nodes[1].addmultisigaddress(
            4,
            [
                addr1Obj['pubkey'],
                addr2Obj['pubkey'],
                addr3Obj['pubkey'],
                addr4Obj['pubkey'],
                addr5Obj['pubkey'],
            ]
        )['address']

        inputs = []
        outputs = {mSigObj: 1100000}
        rawTx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawTx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendtoaddress(mSigObj, 1100000)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

    def test_spend_2of2(self):
        """Spend a 2-of-2 multisig transaction over fundraw."""
        self.log.info("Test fundrawtxn spending 2-of-2 multisig")

        # Create 2-of-2 addr.
        addr1 = self.nodes[2].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[2].getaddressinfo(addr1)
        addr2Obj = self.nodes[2].getaddressinfo(addr2)

        mSigObj = self.nodes[2].addmultisigaddress(
            2,
            [
                addr1Obj['pubkey'],
                addr2Obj['pubkey'],
            ]
        )['address']

        # Send 1.2 BCH to msig addr.
        self.nodes[0].sendtoaddress(mSigObj, 1200000)
        self.nodes[0].generate(1)
        self.sync_all()

        oldBalance = self.nodes[1].getbalance()
        inputs = []
        outputs = {self.nodes[1].getnewaddress(): 1100000}
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[2].fundrawtransaction(rawTx)

        signedTx = self.nodes[2].signrawtransactionwithwallet(fundedTx['hex'])
        self.nodes[2].sendrawtransaction(signedTx['hex'])
        self.nodes[2].generate(1)
        self.sync_all()

        # Make sure funds are received at node1.
        assert_equal(
            oldBalance + Decimal('1100000.00'), self.nodes[1].getbalance())

    def test_locked_wallet(self):
        self.log.info("Test fundrawtxn with locked wallet")

        self.nodes[1].encryptwallet("test")

        # Drain the keypool.
        self.nodes[1].getnewaddress()
        self.nodes[1].getrawchangeaddress()
        inputs = []
        outputs = {self.nodes[0].getnewaddress(): 1099997.00}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        # fund a transaction that does not require a new key for the change
        # output
        self.nodes[1].fundrawtransaction(rawtx)

        # fund a transaction that requires a new key for the change output
        # creating the key must be impossible because the wallet is locked
        outputs = {self.nodes[0].getnewaddress(): 1100000}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(
            -4,
            "Transaction needs a change address, but we can't generate it. Please call keypoolrefill first.",
            self.nodes[1].fundrawtransaction,
            rawtx)

        # Refill the keypool.
        self.nodes[1].walletpassphrase("test", 100)
        # need to refill the keypool to get an internal change address
        self.nodes[1].keypoolrefill(8)
        self.nodes[1].walletlock()

        assert_raises_rpc_error(-13, "walletpassphrase", self.nodes[
            1].sendtoaddress, self.nodes[0].getnewaddress(), 1200000)

        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress(): 1100000}
        rawTx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawTx)

        # Now we need to unlock.
        self.nodes[1].walletpassphrase("test", 600)
        signedTx = self.nodes[1].signrawtransactionwithwallet(fundedTx['hex'])
        self.nodes[1].sendrawtransaction(signedTx['hex'])
        self.nodes[1].generate(1)
        self.sync_all()

        # Make sure funds are received at node1.
        assert_equal(
            oldBalance + Decimal('51100000.00'), self.nodes[0].getbalance())

    def test_many_inputs_fee(self):
        """Multiple (~19) inputs tx test | Compare fee."""
        self.log.info("Test fundrawtxn fee with many inputs")

        # Empty node1, send some small coins from node0 to node1.
        self.nodes[1].sendtoaddress(
            self.nodes[0].getnewaddress(), self.nodes[1].getbalance(), "", "", True)
        self.nodes[1].generate(1)
        self.sync_all()

        for _ in range(0, 20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 10000)
        self.nodes[0].generate(1)
        self.sync_all()

        # Fund a tx with ~20 small inputs.
        inputs = []
        outputs = {
            self.nodes[0].getnewaddress(): 150000, self.nodes[0].getnewaddress(): 40000}
        rawTx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawTx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[1].sendmany("", outputs)
        signedFee = self.nodes[1].getrawmempool(True)[txId]['fee']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        # ~19 inputs
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance * 19

    def test_many_inputs_send(self):
        """Multiple (~19) inputs tx test | sign/send."""
        self.log.info("Test fundrawtxn sign+send with many inputs")

        # Again, empty node1, send some small coins from node0 to node1.
        self.nodes[1].sendtoaddress(
            self.nodes[0].getnewaddress(), self.nodes[1].getbalance(), "", "", True)
        self.nodes[1].generate(1)
        self.sync_all()

        for _ in range(0, 20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 10000)
        self.nodes[0].generate(1)
        self.sync_all()

        # Fund a tx with ~20 small inputs.
        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {
            self.nodes[0].getnewaddress(): 150000, self.nodes[0].getnewaddress(): 40000}
        rawTx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawTx)
        fundedAndSignedTx = self.nodes[1].signrawtransactionwithwallet(
            fundedTx['hex'])
        self.nodes[1].sendrawtransaction(fundedAndSignedTx['hex'])
        self.nodes[1].generate(1)
        self.sync_all()
        assert_equal(oldBalance + Decimal('50190000.00'),
                     self.nodes[0].getbalance())  # 0.19+block reward

    def test_op_return(self):
        self.log.info("Test fundrawtxn with OP_RETURN and no vin")

        rawTx = "0100000000010000000000000000066a047465737400000000"
        dec_tx = self.nodes[2].decoderawtransaction(rawTx)

        assert_equal(len(dec_tx['vin']), 0)
        assert_equal(len(dec_tx['vout']), 1)

        rawtxfund = self.nodes[2].fundrawtransaction(rawTx)
        dec_tx = self.nodes[2].decoderawtransaction(rawtxfund['hex'])

        # at least one vin
        assert_greater_than(len(dec_tx['vin']), 0)
        # one change output added
        assert_equal(len(dec_tx['vout']), 2)

    def test_watchonly(self):
        self.log.info("Test fundrawtxn using only watchonly")

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): self.watchonly_amount / 2}
        rawTx = self.nodes[3].createrawtransaction(inputs, outputs)

        result = self.nodes[3].fundrawtransaction(
            rawTx, {'includeWatching': True})
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 1)
        assert_equal(res_dec["vin"][0]["txid"], self.watchonly_txid)

        assert "fee" in result.keys()
        assert_greater_than(result["changepos"], -1)

    def test_all_watched_funds(self):
        self.log.info("Test fundrawtxn using entirety of watched funds")

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): self.watchonly_amount}
        rawTx = self.nodes[3].createrawtransaction(inputs, outputs)

        # Backward compatibility test (2nd param is includeWatching).
        result = self.nodes[3].fundrawtransaction(rawTx, True)
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 2)
        assert res_dec["vin"][0]["txid"] == self.watchonly_txid or res_dec[
            "vin"][1]["txid"] == self.watchonly_txid

        assert_greater_than(result["fee"], 0)
        assert_greater_than(result["changepos"], -1)
        assert_equal(result["fee"] + res_dec["vout"][
                     result["changepos"]]["value"], self.watchonly_amount / 10)

        signedtx = self.nodes[3].signrawtransactionwithwallet(result["hex"])
        assert not signedtx["complete"]
        signedtx = self.nodes[0].signrawtransactionwithwallet(signedtx["hex"])
        assert signedtx["complete"]
        self.nodes[0].sendrawtransaction(signedtx["hex"])
        self.nodes[0].generate(1)
        self.sync_all()

    def test_option_feerate(self):
        self.log.info("Test fundrawtxn feeRate option")

        # Make sure there is exactly one input so coin selection can't skew the
        # result.
        assert_equal(len(self.nodes[3].listunspent(1)), 1)

        inputs = []
        outputs = {self.nodes[3].getnewaddress(): 1000000}
        rawTx = self.nodes[3].createrawtransaction(inputs, outputs)
        # uses self.min_relay_tx_fee (set by settxfee)
        result = self.nodes[3].fundrawtransaction(rawTx)
        result2 = self.nodes[3].fundrawtransaction(
            rawTx, {"feeRate": 2 * self.min_relay_tx_fee})
        result_fee_rate = result['fee'] * 1000 / \
            FromHex(CTransaction(), result['hex']).billable_size()
        assert_fee_amount(
            result2['fee'], FromHex(CTransaction(), result2['hex']).billable_size(), 2 * result_fee_rate)

        result3 = self.nodes[3].fundrawtransaction(
            rawTx, {"feeRate": 10 * self.min_relay_tx_fee})
        assert_raises_rpc_error(-4,
                                "Fee exceeds maximum configured by -maxtxfee",
                                self.nodes[3].fundrawtransaction,
                                rawTx,
                                {"feeRate": 1000000})
        # allow this transaction to be underfunded by 10 bytes. This is due
        # to the first transaction possibly being overfunded by up to .9
        # satoshi due to  fee ceilings being used.
        assert_fee_amount(
            result3['fee'], FromHex(CTransaction(), result3['hex']).billable_size(), 10 * result_fee_rate, 10)

    def test_address_reuse(self):
        """Test no address reuse occurs."""
        self.log.info("Test fundrawtxn does not reuse addresses")

        rawTx = self.nodes[3].createrawtransaction(
            inputs=[], outputs={self.nodes[3].getnewaddress(): 1000000})
        result3 = self.nodes[3].fundrawtransaction(rawTx)
        res_dec = self.nodes[0].decoderawtransaction(result3["hex"])
        changeaddress = ""
        for out in res_dec['vout']:
            if out['value'] > 1000000.0:
                changeaddress += out['scriptPubKey']['addresses'][0]
        assert changeaddress != ""
        nextaddr = self.nodes[3].getnewaddress()
        # Now the change address key should be removed from the keypool.
        assert changeaddress != nextaddr

    def test_option_subtract_fee_from_outputs(self):
        self.log.info("Test fundrawtxn subtractFeeFromOutputs option")

        # Make sure there is exactly one input so coin selection can't skew the
        # result.
        assert_equal(len(self.nodes[3].listunspent(1)), 1)

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): 1000000}
        rawTx = self.nodes[3].createrawtransaction(inputs, outputs)

        # uses self.min_relay_tx_fee (set by settxfee)
        result = [self.nodes[3].fundrawtransaction(rawTx),
                  # empty subtraction list
                  self.nodes[3].fundrawtransaction(
                      rawTx, {"subtractFeeFromOutputs": []}),
                  # uses self.min_relay_tx_fee (set by settxfee)
                  self.nodes[3].fundrawtransaction(
                      rawTx, {"subtractFeeFromOutputs": [0]}),
                  self.nodes[3].fundrawtransaction(
                      rawTx, {"feeRate": 2 * self.min_relay_tx_fee}),
                  self.nodes[3].fundrawtransaction(
                      rawTx, {"feeRate": 2 * self.min_relay_tx_fee, "subtractFeeFromOutputs": [0]}),
                  ]

        dec_tx = [self.nodes[3].decoderawtransaction(tx_['hex'])
                  for tx_ in result]
        output = [d['vout'][1 - r['changepos']]['value']
                  for d, r in zip(dec_tx, result)]
        change = [d['vout'][r['changepos']]['value']
                  for d, r in zip(dec_tx, result)]

        assert_equal(result[0]['fee'], result[1]['fee'], result[2]['fee'])
        assert_equal(result[3]['fee'], result[4]['fee'])
        assert_equal(change[0], change[1])
        assert_equal(output[0], output[1])
        assert_equal(output[0], output[2] + result[2]['fee'])
        assert_equal(change[0] + result[0]['fee'], change[2])
        assert_equal(output[3], output[4] + result[4]['fee'])
        assert_equal(change[3] + result[3]['fee'], change[4])

        inputs = []
        outputs = {
            self.nodes[2].getnewaddress(): value for value in (1000000.0, 1100000.0, 1200000.0, 1300000.0)}
        rawTx = self.nodes[3].createrawtransaction(inputs, outputs)

        # Split the fee between outputs 0, 2, and 3, but not output 1
        result = [self.nodes[3].fundrawtransaction(rawTx),
                  self.nodes[3].fundrawtransaction(rawTx, {"subtractFeeFromOutputs": [0, 2, 3]})]

        dec_tx = [self.nodes[3].decoderawtransaction(result[0]['hex']),
                  self.nodes[3].decoderawtransaction(result[1]['hex'])]

        # Nested list of non-change output amounts for each transaction.
        output = [[out['value'] for i, out in enumerate(d['vout']) if i != r['changepos']]
                  for d, r in zip(dec_tx, result)]

        # List of differences in output amounts between normal and subtractFee
        # transactions.
        share = [o0 - o1 for o0, o1 in zip(output[0], output[1])]

        # Output 1 is the same in both transactions.
        assert_equal(share[1], 0)

        # The other 3 outputs are smaller as a result of
        # subtractFeeFromOutputs.
        assert_greater_than(share[0], 0)
        assert_greater_than(share[2], 0)
        assert_greater_than(share[3], 0)

        # Outputs 2 and 3 take the same share of the fee.
        assert_equal(share[2], share[3])

        # Output 0 takes at least as much share of the fee, and no more than 2
        # satoshis more, than outputs 2 and 3.
        assert_greater_than_or_equal(share[0], share[2])
        assert_greater_than_or_equal(share[2] + Decimal(2e-8), share[0])

        # The fee is the same in both transactions.
        assert_equal(result[0]['fee'], result[1]['fee'])

        # The total subtracted from the outputs is equal to the fee.
        assert_equal(share[0] + share[2] + share[3], result[0]['fee'])

    def test_subtract_fee_with_presets(self):
        self.log.info(
            "Test fundrawtxn subtract fee from outputs with preset inputs that are sufficient")

        addr = self.nodes[0].getnewaddress()
        txid = self.nodes[0].sendtoaddress(addr, 10000000)
        vout = find_vout_for_address(self.nodes[0], txid, addr)

        rawtx = self.nodes[0].createrawtransaction([{'txid': txid, 'vout': vout}], [
                                                   {self.nodes[0].getnewaddress(): 5000000}])
        fundedtx = self.nodes[0].fundrawtransaction(
            rawtx, {'subtractFeeFromOutputs': [0]})
        signedtx = self.nodes[0].signrawtransactionwithwallet(fundedtx['hex'])
        self.nodes[0].sendrawtransaction(signedtx['hex'])


if __name__ == '__main__':
    RawTransactionsTest().main()
