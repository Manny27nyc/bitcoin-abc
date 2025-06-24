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
"""Test the scantxoutset rpc call."""
import os
import shutil
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


def descriptors(out):
    return sorted(u['desc'] for u in out['unspents'])


class ScantxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(110)

        addr = self.nodes[0].getnewaddress("")
        pubkey = self.nodes[0].getaddressinfo(addr)['pubkey']
        self.nodes[0].sendtoaddress(addr, 2000)

        # send to child keys of tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK
        # (m/0'/0'/0')
        self.nodes[0].sendtoaddress(
            "mkHV1C6JLheLoUSSZYk7x3FH5tnx9bu7yc", 8000)
        # (m/0'/0'/1')
        self.nodes[0].sendtoaddress(
            "mipUSRmJAj2KrjSvsPQtnP8ynUon7FhpCR", 16000)
        # (m/0'/0'/1500')
        self.nodes[0].sendtoaddress(
            "n37dAGe6Mq1HGM9t4b6rFEEsDGq7Fcgfqg", 32000)
        # (m/0'/0'/0)
        self.nodes[0].sendtoaddress(
            "mqS9Rpg8nNLAzxFExsgFLCnzHBsoQ3PRM6", 64000)
        # (m/0'/0'/1)
        self.nodes[0].sendtoaddress(
            "mnTg5gVWr3rbhHaKjJv7EEEc76ZqHgSj4S", 128000)
        # (m/0'/0'/1500)
        self.nodes[0].sendtoaddress(
            "mketCd6B9U9Uee1iCsppDJJBHfvi6U6ukC", 256000)
        # (m/1/1/0')
        self.nodes[0].sendtoaddress(
            "mj8zFzrbBcdaWXowCQ1oPZ4qioBVzLzAp7", 512000)
        # (m/1/1/1')
        self.nodes[0].sendtoaddress(
            "mfnKpKQEftniaoE1iXuMMePQU3PUpcNisA", 1024000)
        # (m/1/1/1500')
        self.nodes[0].sendtoaddress(
            "mou6cB1kaP1nNJM1sryW6YRwnd4shTbXYQ", 2048000)
        # (m/1/1/0)
        self.nodes[0].sendtoaddress(
            "mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ", 4096000)
        # (m/1/1/1)
        self.nodes[0].sendtoaddress(
            "mxp7w7j8S1Aq6L8StS2PqVvtt4HGxXEvdy", 8192000)
        # (m/1/1/1500)
        self.nodes[0].sendtoaddress(
            "mpQ8rokAhp1TAtJQR6F6TaUmjAWkAWYYBq", 16384000)

        self.nodes[0].generate(1)

        self.log.info("Stop node, remove wallet, mine again some blocks...")
        self.stop_node(0)
        shutil.rmtree(os.path.join(
            self.nodes[0].datadir, self.chain, 'wallets'))
        self.start_node(0)
        self.nodes[0].generate(110)

        scan = self.nodes[0].scantxoutset("start", [])
        info = self.nodes[0].gettxoutsetinfo()
        assert_equal(scan['success'], True)
        assert_equal(scan['height'], info['height'])
        assert_equal(scan['txouts'], info['txouts'])
        assert_equal(scan['bestblock'], info['bestblock'])

        self.restart_node(0, ['-nowallet'])
        self.log.info("Test if we have found the non HD unspent outputs.")
        assert_equal(self.nodes[0].scantxoutset(
            "start", ["pkh(" + pubkey + ")"])['total_amount'], Decimal("2000"))
        assert_equal(self.nodes[0].scantxoutset(
            "start", ["combo(" + pubkey + ")"])['total_amount'], Decimal("2000"))
        assert_equal(self.nodes[0].scantxoutset(
            "start", ["addr(" + addr + ")"])['total_amount'], Decimal("2000"))
        assert_equal(self.nodes[0].scantxoutset(
            "start", ["addr(" + addr + ")"])['total_amount'], Decimal("2000"))

        self.log.info("Test range validation.")
        assert_raises_rpc_error(-8,
                                "End of range is too high",
                                self.nodes[0].scantxoutset,
                                "start",
                                [{"desc": "desc",
                                  "range": -1}])
        assert_raises_rpc_error(-8,
                                "Range should be greater or equal than 0",
                                self.nodes[0].scantxoutset,
                                "start",
                                [{"desc": "desc",
                                  "range": [-1,
                                            10]}])
        assert_raises_rpc_error(-8,
                                "End of range is too high",
                                self.nodes[0].scantxoutset,
                                "start",
                                [{"desc": "desc",
                                  "range": [(2 << 31 + 1) - 1000000,
                                            (2 << 31 + 1)]}])
        assert_raises_rpc_error(-8,
                                "Range specified as [begin,end] must not have begin after end",
                                self.nodes[0].scantxoutset,
                                "start",
                                [{"desc": "desc",
                                  "range": [2,
                                            1]}])
        assert_raises_rpc_error(-8,
                                "Range is too large",
                                self.nodes[0].scantxoutset,
                                "start",
                                [{"desc": "desc",
                                  "range": [0,
                                            1000001]}])

        self.log.info("Test extended key derivation.")
        # Run various scans, and verify that the sum of the amounts of the matches corresponds to the expected subset.
        # Note that all amounts in the UTXO set are powers of 2 multiplied by
        # 0.001 BTC, so each amounts uniquely identifies a subset.
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/0h)"])['total_amount'], Decimal("8000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0'/1h)"])['total_amount'], Decimal("16000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/1500')"])['total_amount'], Decimal("32000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0h/0)"])['total_amount'], Decimal("64000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/1)"])['total_amount'], Decimal("128000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/1500)"])['total_amount'], Decimal("256000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/*h)", "range": 1499}])['total_amount'], Decimal("24000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0'/*h)", "range": 1500}])['total_amount'], Decimal("56000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/*)", "range": 1499}])['total_amount'], Decimal("192000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0'/0h/*)", "range": 1500}])['total_amount'], Decimal("448000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0')"])['total_amount'], Decimal("0512000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1')"])['total_amount'], Decimal("1024000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1500h)"])['total_amount'], Decimal("2048000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)"])['total_amount'], Decimal("4096000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1)"])['total_amount'], Decimal("8192000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/1500)"])['total_amount'], Decimal("16384000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/0)"])['total_amount'], Decimal("4096000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo([abcdef88/1/2'/3/4h]tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/1)"])['total_amount'], Decimal("8192000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/1500)"])['total_amount'], Decimal("16384000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*')", "range": 1499}])['total_amount'], Decimal("1536000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*')", "range": 1500}])['total_amount'], Decimal("3584000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)", "range": 1499}])['total_amount'], Decimal("12288000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/*)", "range": 1500}])['total_amount'], Decimal("28672000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/*)", "range": 1499}])['total_amount'], Decimal("12288000"))
        assert_equal(self.nodes[0].scantxoutset("start", [
                     {"desc": "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/*)", "range": 1500}])['total_amount'], Decimal("28672000"))
        assert_equal(
            self.nodes[0].scantxoutset(
                "start",
                [
                    {
                        "desc": "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/*)",
                        "range": [
                            1500,
                            1500]}])['total_amount'],
            Decimal("16384000"))

        # Test the reported descriptors for a few matches
        assert_equal(descriptors(self.nodes[0].scantxoutset("start",
                                                            [{"desc": "combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/0'/*)",
                                                              "range": 1499}])),
                     ["pkh([0c5f9a1e/0'/0'/0]026dbd8b2315f296d36e6b6920b1579ca75569464875c7ebe869b536a7d9503c8c)#dzxw429x",
                      "pkh([0c5f9a1e/0'/0'/1]033e6f25d76c00bedb3a8993c7d5739ee806397f0529b1b31dda31ef890f19a60c)#43rvceed"])
        assert_equal(
            descriptors(
                self.nodes[0].scantxoutset(
                    "start",
                    ["combo(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/1/1/0)"])),
            ["pkh([0c5f9a1e/1/1/0]03e1c5b6e650966971d7e71ef2674f80222752740fc1dfd63bbbd220d2da9bd0fb)#cxmct4w8"])
        assert_equal(descriptors(self.nodes[0].scantxoutset("start",
                                                            [{"desc": "combo(tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/1/1/*)",
                                                              "range": 1500}])),
                     ['pkh([0c5f9a1e/1/1/0]03e1c5b6e650966971d7e71ef2674f80222752740fc1dfd63bbbd220d2da9bd0fb)#cxmct4w8',
                      'pkh([0c5f9a1e/1/1/1500]03832901c250025da2aebae2bfb38d5c703a57ab66ad477f9c578bfbcd78abca6f)#vchwd07g',
                      'pkh([0c5f9a1e/1/1/1]030d820fc9e8211c4169be8530efbc632775d8286167afd178caaf1089b77daba7)#z2t3ypsa'])

        # Check that status and abort don't need second arg
        assert_equal(self.nodes[0].scantxoutset("status"), None)
        assert_equal(self.nodes[0].scantxoutset("abort"), False)

        # Check that second arg is needed for start
        assert_raises_rpc_error(-1,
                                "scanobjects argument is required for the start action",
                                self.nodes[0].scantxoutset,
                                "start")


if __name__ == '__main__':
    ScantxoutsetTest().main()
