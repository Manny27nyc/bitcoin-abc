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
"""Test the importdescriptors RPC.

Test importdescriptors by generating keys on node0, importing the corresponding
descriptors on node1 and then testing the address info for the different address
variants.

- `get_generate_key()` is called to generate keys and return the privkeys,
  pubkeys and all variants of scriptPubKey and address.
- `test_importdesc()` is called to send an importdescriptors call to node1, test
  success, and (if unsuccessful) test the error code and error message returned.
- `test_address()` is called to call getaddressinfo for an address on node1
  and test the values returned."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)
from test_framework.wallet_util import (
    get_generate_key,
    test_address,
)


class ImportDescriptorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], ["-keypool=5"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_importdesc(self, req, success, error_code=None,
                        error_message=None, warnings=None, wallet=None):
        """Run importdescriptors and assert success"""
        if warnings is None:
            warnings = []
        wrpc = self.nodes[1].get_wallet_rpc('w1')
        if wallet is not None:
            wrpc = wallet

        result = wrpc.importdescriptors([req])
        observed_warnings = []
        if 'warnings' in result[0]:
            observed_warnings = result[0]['warnings']
        assert_equal(
            "\n".join(
                sorted(warnings)), "\n".join(
                sorted(observed_warnings)))
        assert_equal(result[0]['success'], success)
        if error_code is not None:
            assert_equal(result[0]['error']['code'], error_code)
            assert_equal(result[0]['error']['message'], error_message)

    def run_test(self):
        self.log.info('Setting up wallets')
        self.nodes[0].createwallet(
            wallet_name='w0',
            disable_private_keys=False)
        w0 = self.nodes[0].get_wallet_rpc('w0')

        self.nodes[1].createwallet(
            wallet_name='w1',
            disable_private_keys=True,
            blank=True,
            descriptors=True)
        w1 = self.nodes[1].get_wallet_rpc('w1')
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        self.nodes[1].createwallet(
            wallet_name="wpriv",
            disable_private_keys=False,
            blank=True,
            descriptors=True)
        wpriv = self.nodes[1].get_wallet_rpc("wpriv")
        assert_equal(wpriv.getwalletinfo()['keypoolsize'], 0)

        self.log.info('Mining coins')
        w0.generatetoaddress(101, w0.getnewaddress())

        # RPC importdescriptors -----------------------------------------------

        # # Test import fails if no descriptor present
        key = get_generate_key()
        self.log.info("Import should fail if a descriptor is not provided")
        self.test_importdesc({"timestamp": "now"},
                             success=False,
                             error_code=-8,
                             error_message='Descriptor not found.')

        # # Test importing of a P2PKH descriptor
        key = get_generate_key()
        self.log.info("Should import a p2pkh descriptor")
        self.test_importdesc({"desc": descsum_create("pkh(" + key.pubkey + ")"),
                              "timestamp": "now",
                              "label": "Descriptor import test"},
                             success=True)
        test_address(w1,
                     key.p2pkh_addr,
                     solvable=True,
                     ismine=True,
                     labels=["Descriptor import test"])
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        self.log.info("Internal addresses cannot have labels")
        self.test_importdesc({"desc": descsum_create("pkh(" + key.pubkey + ")"),
                              "timestamp": "now",
                              "internal": True,
                              "label": "Descriptor import test"},
                             success=False,
                             error_code=-8,
                             error_message="Internal addresses should not have a label")

        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        test_address(w1,
                     key.p2pkh_addr,
                     ismine=True,
                     solvable=True)

        # # Test importing of a multisig descriptor
        key1 = get_generate_key()
        key2 = get_generate_key()
        self.log.info("Should import a 1-of-2 bare multisig from descriptor")
        self.test_importdesc({"desc": descsum_create("multi(1," + key1.pubkey + "," + key2.pubkey + ")"),
                              "timestamp": "now"},
                             success=True)
        self.log.info(
            "Should not treat individual keys from the imported bare multisig as watchonly")
        test_address(w1,
                     key1.p2pkh_addr,
                     ismine=False)

        # # Test ranged descriptors
        xpriv = "tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg"
        xpub = "tpubD6NzVbkrYhZ4YNXVQbNhMK1WqguFsUXceaVJKbmno2aZ3B6QfbMeraaYvnBSGpV3vxLyTTK9DYT1yoEck4XUScMzXoQ2U2oSmE2JyMedq3H"
        addresses = [
            "2N7yv4p8G8yEaPddJxY41kPihnWvs39qCMf",
            "2MsHxyb2JS3pAySeNUsJ7mNnurtpeenDzLA"]  # hdkeypath=m/0'/0'/0' and 1'
        # wpkh subscripts corresponding to the above addresses
        addresses += ["ecregtest:prvn9ycvgr5atuyh49sua3mapskh2mnnzg7t9yp6dt",
                      "ecregtest:pp3n087yx0njv2e5wcvltahfxqst7l66rutz8ceeat"]
        desc = "sh(pkh(" + xpub + "/0/0/*" + "))"

        self.log.info("Ranged descriptors cannot have labels")
        self.test_importdesc({"desc": descsum_create(desc),
                              "timestamp": "now",
                              "range": [0, 100],
                              "label": "test"},
                             success=False,
                             error_code=-8,
                             error_message='Ranged descriptors should not have a label')

        self.log.info("Private keys required for private keys enabled wallet")
        self.test_importdesc({"desc": descsum_create(desc),
                              "timestamp": "now",
                              "range": [0, 100]},
                             success=False,
                             error_code=-4,
                             error_message='Cannot import descriptor without private keys to a wallet with private keys enabled',
                             wallet=wpriv)

        self.log.info(
            "Ranged descriptor import should warn without a specified range")
        self.test_importdesc({"desc": descsum_create(desc),
                              "timestamp": "now"},
                             success=True,
                             warnings=['Range not given, using default keypool range'])
        assert_equal(w1.getwalletinfo()['keypoolsize'], 0)

        # # Test importing of a ranged descriptor with xpriv
        self.log.info(
            "Should not import a ranged descriptor that includes xpriv into a watch-only wallet")
        desc = "sh(pkh(" + xpriv + "/0'/0'/*'" + "))"
        self.test_importdesc({"desc": descsum_create(desc),
                              "timestamp": "now",
                              "range": 1},
                             success=False,
                             error_code=-4,
                             error_message='Cannot import private keys to a wallet with private keys disabled')

        for address in addresses:
            test_address(w1,
                         address,
                         ismine=False,
                         solvable=False)

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": -1},
                             success=False, error_code=-8, error_message='End of range is too high')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [-1, 10]},
                             success=False, error_code=-8, error_message='Range should be greater or equal than 0')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [(2 << 31 + 1) - 1000000, (2 << 31 + 1)]},
                             success=False, error_code=-8, error_message='End of range is too high')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [2, 1]},
                             success=False, error_code=-8, error_message='Range specified as [begin,end] must not have begin after end')

        self.test_importdesc({"desc": descsum_create(desc), "timestamp": "now", "range": [0, 1000001]},
                             success=False, error_code=-8, error_message='Range is too large')

        # Make sure ranged imports import keys in order
        w1 = self.nodes[1].get_wallet_rpc('w1')
        self.log.info('Key ranges should be imported in order')
        xpub = "tpubDAXcJ7s7ZwicqjprRaEWdPoHKrCS215qxGYxpusRLLmJuT69ZSicuGdSfyvyKpvUNYBW1s2U3NSrT6vrCYB9e6nZUEvrqnwXPF8ArTCRXMY"
        addresses = [
            'ecregtest:qp0v86h53rc92hjrlpwzpjtdlgzsxu25svv6g40fpl',  # m/0'/0'/0
            'ecregtest:qqasy0zlkdleqt4pkn8fs4ehm5gnnz6qpgdcpt90fq',  # m/0'/0'/1
            'ecregtest:qp0sp4wlhctvprqvdt2dgvqcfdjssu04xgey0l3syw',  # m/0'/0'/2
            'ecregtest:qrhn24tegn04cptfv4ldhtkduxq55zcwrycjfdj9vr',  # m/0'/0'/3
            'ecregtest:qzpqhett2uwltq803vrxv7zkqhft5vsnmcjeh50v0p',  # m/0'/0'/4
        ]

        self.test_importdesc({'desc': descsum_create('sh(pkh([abcdef12/0h/0h]' + xpub + '/*))'),
                              'active': True,
                              'range': [0, 2],
                              'timestamp': 'now'
                              },
                             success=True)
        self.test_importdesc({'desc': descsum_create('pkh([12345678/0h/0h]' + xpub + '/*)'),
                              'active': True,
                              'range': [0, 2],
                              'timestamp': 'now'
                              },
                             success=True)

        assert_equal(w1.getwalletinfo()['keypoolsize'], 5)
        for i, expected_addr in enumerate(addresses):
            pkh_addr = w1.getnewaddress('')
            assert_raises_rpc_error(-4,
                                    'This wallet has no available keys',
                                    w1.getrawchangeaddress)

            assert_equal(pkh_addr, expected_addr)
            pkh_addr_info = w1.getaddressinfo(pkh_addr)
            assert_equal(pkh_addr_info['desc'][:22],
                         'pkh([12345678/0\'/0\'/{}]'.format(i))

            # After retrieving a key, we don't refill the keypool again, so
            # it's one less for each address type
            assert_equal(w1.getwalletinfo()['keypoolsize'], 4)
        w1.keypoolrefill()
        assert_equal(w1.getwalletinfo()['keypoolsize'], 5)

        # Check active=False default
        self.log.info('Check imported descriptors are not active by default')
        self.test_importdesc({'desc': descsum_create('pkh([12345678/0h/0h]' + xpub + '/*)'),
                              'range': [0, 2],
                              'timestamp': 'now',
                              'internal': True
                              },
                             success=True)
        assert_raises_rpc_error(-4,
                                'This wallet has no available keys',
                                w1.getrawchangeaddress)

        # # Test importing a descriptor containing a WIF private key
        wif_priv = "cTe1f5rdT8A8DFgVWTjyPwACsDPJM9ff4QngFxUixCSvvbg1x6sh"
        address = "ecregtest:ppn85zpvym8cdccmgw8km6e48jfhnpa435h3hkyfd6"
        desc = "sh(pkh(" + wif_priv + "))"
        self.log.info(
            "Should import a descriptor with a WIF private key as spendable")
        self.test_importdesc({"desc": descsum_create(desc),
                              "timestamp": "now"},
                             success=True,
                             wallet=wpriv)
        test_address(wpriv,
                     address,
                     solvable=True,
                     ismine=True)
        txid = w0.sendtoaddress(address, 49999996.00)
        w0.generatetoaddress(6, w0.getnewaddress())
        self.sync_blocks()
        tx = wpriv.createrawtransaction([{"txid": txid, "vout": 0}], {
                                        w0.getnewaddress(): 49999000})
        signed_tx = wpriv.signrawtransactionwithwallet(tx)
        w1.sendrawtransaction(signed_tx['hex'])

        # Make sure that we can use import and use multisig as addresses
        self.log.info(
            'Test that multisigs can be imported, signed for, and getnewaddress\'d')
        self.nodes[1].createwallet(
            wallet_name="wmulti_priv",
            disable_private_keys=False,
            blank=True,
            descriptors=True)
        wmulti_priv = self.nodes[1].get_wallet_rpc("wmulti_priv")
        assert_equal(wmulti_priv.getwalletinfo()['keypoolsize'], 0)

        self.test_importdesc({"desc": "sh(multi(2,tprv8ZgxMBicQKsPevADjDCWsa6DfhkVXicu8NQUzfibwX2MexVwW4tCec5mXdCW8kJwkzBRRmAay1KZya4WsehVvjTGVW6JLqiqd8DdZ4xSg52/84h/0h/0h/*,tprv8ZgxMBicQKsPdSNWUhDiwTScDr6JfkZuLshTRwzvZGnMSnGikV6jxpmdDkC3YRc4T3GD6Nvg9uv6hQg73RVv1EiTXDZwxVbsLugVHU8B1aq/84h/0h/0h/*,tprv8ZgxMBicQKsPeonDt8Ka2mrQmHa61hQ5FQCsvWBTpSNzBFgM58cV2EuXNAHF14VawVpznnme3SuTbA62sGriwWyKifJmXntfNeK7zeqMCj1/84h/0h/0h/*))#f5nqn4ax",
                              "active": True,
                              "range": 1000,
                              "next_index": 0,
                              "timestamp": "now"},
                             success=True,
                             wallet=wmulti_priv)
        self.test_importdesc({"desc": "sh(multi(2,tprv8ZgxMBicQKsPevADjDCWsa6DfhkVXicu8NQUzfibwX2MexVwW4tCec5mXdCW8kJwkzBRRmAay1KZya4WsehVvjTGVW6JLqiqd8DdZ4xSg52/84h/1h/0h/*,tprv8ZgxMBicQKsPdSNWUhDiwTScDr6JfkZuLshTRwzvZGnMSnGikV6jxpmdDkC3YRc4T3GD6Nvg9uv6hQg73RVv1EiTXDZwxVbsLugVHU8B1aq/84h/1h/0h/*,tprv8ZgxMBicQKsPeonDt8Ka2mrQmHa61hQ5FQCsvWBTpSNzBFgM58cV2EuXNAHF14VawVpznnme3SuTbA62sGriwWyKifJmXntfNeK7zeqMCj1/84h/1h/0h/*))#m4e4s5de",
                              "active": True,
                              "internal": True,
                              "range": 1000,
                              "next_index": 0,
                              "timestamp": "now"},
                             success=True,
                             wallet=wmulti_priv)

        # Range end (1000) is inclusive, so 1001 addresses generated
        assert_equal(wmulti_priv.getwalletinfo()['keypoolsize'], 1001)
        addr = wmulti_priv.getnewaddress('')
        # Derived at m/84'/0'/0'/0
        assert_equal(
            addr,
            'ecregtest:pzkcf26dw7np58jcspnpxaupgz9csnc3wsf5wdh2a3')
        change_addr = wmulti_priv.getrawchangeaddress()
        assert_equal(
            change_addr,
            'ecregtest:prnkfg7pxe3kpyv3l4v00ft6q3sfseag7vnva0k49n')

        assert_equal(wmulti_priv.getwalletinfo()['keypoolsize'], 1000)
        txid = w0.sendtoaddress(addr, 10000000)
        self.nodes[0].generate(6)
        self.sync_all()

        self.nodes[1].createwallet(
            wallet_name="wmulti_pub",
            disable_private_keys=True,
            blank=True,
            descriptors=True)
        wmulti_pub = self.nodes[1].get_wallet_rpc("wmulti_pub")
        assert_equal(wmulti_pub.getwalletinfo()['keypoolsize'], 0)

        self.test_importdesc({"desc": "sh(multi(2,[7b2d0242/84h/0h/0h]tpubDCJtdt5dgJpdhW4MtaVYDhG4T4tF6jcLR1PxL43q9pq1mxvXgMS9Mzw1HnXG15vxUGQJMMSqCQHMTy3F1eW5VkgVroWzchsPD5BUojrcWs8/*,[59b09cd6/84h/0h/0h]tpubDDBF2BTR6s8drwrfDei8WxtckGuSm1cyoKxYY1QaKSBFbHBYQArWhHPA6eJrzZej6nfHGLSURYSLHr7GuYch8aY5n61tGqgn8b4cXrMuoPH/*,[e81a0532/84h/0h/0h]tpubDCsWoW1kuQB9kG5MXewHqkbjPtqPueRnXju7uM2NK7y3JYb2ajAZ9EiuZXNNuE4661RAfriBWhL8UsnAPpk8zrKKnZw1Ug7X4oHgMdZiU4E/*))#x75vpsak",
                              "active": True,
                              "range": 1000,
                              "next_index": 0,
                              "timestamp": "now"},
                             success=True,
                             wallet=wmulti_pub)
        self.test_importdesc({"desc": "sh(multi(2,[7b2d0242/84h/1h/0h]tpubDCXqdwWZcszwqYJSnZp8eARkxGJfHAk23KDxbztV4BbschfaTfYLTcSkSJ3TN64dRqwa1rnFUScsYormKkGqNbbPwkorQimVevXjxzUV9Gf/*,[59b09cd6/84h/1h/0h]tpubDCYfZY2ceyHzYzMMVPt9MNeiqtQ2T7Uyp9QSFwYXh8Vi9iJFYXcuphJaGXfF3jUQJi5Y3GMNXvM11gaL4txzZgNGK22BFAwMXynnzv4z2Jh/*,[e81a0532/84h/1h/0h]tpubDC6UGqnsQStngYuGD4MKsMy7eD1Yg9NTJfPdvjdG2JE5oZ7EsSL3WHg4Gsw2pR5K39ZwJ46M1wZayhedVdQtMGaUhq5S23PH6fnENK3V1sb/*))#v0t48ucu",
                              "active": True,
                              "internal": True,
                              "range": 1000,
                              "next_index": 0,
                              "timestamp": "now"},
                             success=True,
                             wallet=wmulti_pub)

        # The first one was already consumed by previous import and is detected
        # as used
        assert_equal(wmulti_pub.getwalletinfo()['keypoolsize'], 1000)
        addr = wmulti_pub.getnewaddress('')
        # Derived at m/84'/0'/0'/1
        assert_equal(
            addr,
            'ecregtest:pr5xql8r03jp5dvrep22dns59vf7hhykr5nmaucy2h')
        change_addr = wmulti_pub.getrawchangeaddress()
        assert_equal(
            change_addr,
            'ecregtest:prnkfg7pxe3kpyv3l4v00ft6q3sfseag7vnva0k49n')

        assert_equal(wmulti_pub.getwalletinfo()['keypoolsize'], 999)
        txid = w0.sendtoaddress(addr, 10000000)
        vout = find_vout_for_address(self.nodes[0], txid, addr)
        self.nodes[0].generate(6)
        self.sync_all()
        assert_equal(wmulti_pub.getbalance(), wmulti_priv.getbalance())

        self.log.info("Multisig with distributed keys")
        self.nodes[1].createwallet(
            wallet_name="wmulti_priv1",
            descriptors=True)
        wmulti_priv1 = self.nodes[1].get_wallet_rpc("wmulti_priv1")
        res = wmulti_priv1.importdescriptors([
            {
                "desc": descsum_create("sh(multi(2,tprv8ZgxMBicQKsPevADjDCWsa6DfhkVXicu8NQUzfibwX2MexVwW4tCec5mXdCW8kJwkzBRRmAay1KZya4WsehVvjTGVW6JLqiqd8DdZ4xSg52/84h/0h/0h/*,[59b09cd6/84h/0h/0h]tpubDDBF2BTR6s8drwrfDei8WxtckGuSm1cyoKxYY1QaKSBFbHBYQArWhHPA6eJrzZej6nfHGLSURYSLHr7GuYch8aY5n61tGqgn8b4cXrMuoPH/*,[e81a0532/84h/0h/0h]tpubDCsWoW1kuQB9kG5MXewHqkbjPtqPueRnXju7uM2NK7y3JYb2ajAZ9EiuZXNNuE4661RAfriBWhL8UsnAPpk8zrKKnZw1Ug7X4oHgMdZiU4E/*))"),
                "active": True,
                "range": 1000,
                "next_index": 0,
                "timestamp": "now"
            },
            {
                "desc": descsum_create("sh(multi(2,tprv8ZgxMBicQKsPevADjDCWsa6DfhkVXicu8NQUzfibwX2MexVwW4tCec5mXdCW8kJwkzBRRmAay1KZya4WsehVvjTGVW6JLqiqd8DdZ4xSg52/84h/1h/0h/*,[59b09cd6/84h/1h/0h]tpubDCYfZY2ceyHzYzMMVPt9MNeiqtQ2T7Uyp9QSFwYXh8Vi9iJFYXcuphJaGXfF3jUQJi5Y3GMNXvM11gaL4txzZgNGK22BFAwMXynnzv4z2Jh/*,[e81a0532/84h/1h/0h]tpubDC6UGqnsQStngYuGD4MKsMy7eD1Yg9NTJfPdvjdG2JE5oZ7EsSL3WHg4Gsw2pR5K39ZwJ46M1wZayhedVdQtMGaUhq5S23PH6fnENK3V1sb/*))"),
                "active": True,
                "internal": True,
                "range": 1000,
                "next_index": 0,
                "timestamp": "now"
            }])
        assert_equal(res[0]['success'], True)
        assert_equal(
            res[0]['warnings'][0],
            'Not all private keys provided. Some wallet functionality may return unexpected errors')
        assert_equal(res[1]['success'], True)
        assert_equal(
            res[1]['warnings'][0],
            'Not all private keys provided. Some wallet functionality may return unexpected errors')

        self.nodes[1].createwallet(
            wallet_name='wmulti_priv2',
            blank=True,
            descriptors=True)
        wmulti_priv2 = self.nodes[1].get_wallet_rpc('wmulti_priv2')
        res = wmulti_priv2.importdescriptors([
            {
                "desc": descsum_create("sh(multi(2,[7b2d0242/84h/0h/0h]tpubDCJtdt5dgJpdhW4MtaVYDhG4T4tF6jcLR1PxL43q9pq1mxvXgMS9Mzw1HnXG15vxUGQJMMSqCQHMTy3F1eW5VkgVroWzchsPD5BUojrcWs8/*,tprv8ZgxMBicQKsPdSNWUhDiwTScDr6JfkZuLshTRwzvZGnMSnGikV6jxpmdDkC3YRc4T3GD6Nvg9uv6hQg73RVv1EiTXDZwxVbsLugVHU8B1aq/84h/0h/0h/*,[e81a0532/84h/0h/0h]tpubDCsWoW1kuQB9kG5MXewHqkbjPtqPueRnXju7uM2NK7y3JYb2ajAZ9EiuZXNNuE4661RAfriBWhL8UsnAPpk8zrKKnZw1Ug7X4oHgMdZiU4E/*))"),
                "active": True,
                "range": 1000,
                "next_index": 0,
                "timestamp": "now"
            },
            {
                "desc": descsum_create("sh(multi(2,[7b2d0242/84h/1h/0h]tpubDCXqdwWZcszwqYJSnZp8eARkxGJfHAk23KDxbztV4BbschfaTfYLTcSkSJ3TN64dRqwa1rnFUScsYormKkGqNbbPwkorQimVevXjxzUV9Gf/*,tprv8ZgxMBicQKsPdSNWUhDiwTScDr6JfkZuLshTRwzvZGnMSnGikV6jxpmdDkC3YRc4T3GD6Nvg9uv6hQg73RVv1EiTXDZwxVbsLugVHU8B1aq/84h/1h/0h/*,[e81a0532/84h/1h/0h]tpubDC6UGqnsQStngYuGD4MKsMy7eD1Yg9NTJfPdvjdG2JE5oZ7EsSL3WHg4Gsw2pR5K39ZwJ46M1wZayhedVdQtMGaUhq5S23PH6fnENK3V1sb/*))"),
                "active": True,
                "internal": True,
                "range": 1000,
                "next_index": 0,
                "timestamp": "now"
            }])
        assert_equal(res[0]['success'], True)
        assert_equal(
            res[0]['warnings'][0],
            'Not all private keys provided. Some wallet functionality may return unexpected errors')
        assert_equal(res[1]['success'], True)
        assert_equal(
            res[1]['warnings'][0],
            'Not all private keys provided. Some wallet functionality may return unexpected errors')

        rawtx = self.nodes[1].createrawtransaction(
            [{'txid': txid, 'vout': vout}], {w0.getnewaddress(): 9999000})
        tx_signed_1 = wmulti_priv1.signrawtransactionwithwallet(rawtx)
        assert_equal(tx_signed_1['complete'], False)
        tx_signed_2 = wmulti_priv2.signrawtransactionwithwallet(
            tx_signed_1['hex'])
        assert_equal(tx_signed_2['complete'], True)
        self.nodes[1].sendrawtransaction(tx_signed_2['hex'])

        self.log.info("Combo descriptors cannot be active")
        self.test_importdesc({"desc": descsum_create("combo(tpubDCJtdt5dgJpdhW4MtaVYDhG4T4tF6jcLR1PxL43q9pq1mxvXgMS9Mzw1HnXG15vxUGQJMMSqCQHMTy3F1eW5VkgVroWzchsPD5BUojrcWs8/*)"),
                              "active": True,
                              "range": 1,
                              "timestamp": "now"},
                             success=False,
                             error_code=-4,
                             error_message="Combo descriptors cannot be set to active")

        self.log.info("Descriptors with no type cannot be active")
        self.test_importdesc({"desc": descsum_create("pk(tpubDCJtdt5dgJpdhW4MtaVYDhG4T4tF6jcLR1PxL43q9pq1mxvXgMS9Mzw1HnXG15vxUGQJMMSqCQHMTy3F1eW5VkgVroWzchsPD5BUojrcWs8/*)"),
                              "active": True,
                              "range": 1,
                              "timestamp": "now"},
                             success=True,
                             warnings=["Unknown output type, cannot set descriptor to active."])


if __name__ == '__main__':
    ImportDescriptorsTest().main()
