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
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Useful util functions for testing the wallet"""
from collections import namedtuple

from test_framework.address import (
    key_to_p2pkh,
    script_to_p2sh,
)
from test_framework.key import (
    bytes_to_wif,
    ECKey,
)
from test_framework.script import (
    CScript,
    OP_2,
    OP_3,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_HASH160,
    hash160,
)
from test_framework.util import hex_str_to_bytes

Key = namedtuple('Key', ['privkey',
                         'pubkey',
                         'p2pkh_script',
                         'p2pkh_addr'])

Multisig = namedtuple('Multisig', ['privkeys',
                                   'pubkeys',
                                   'p2sh_script',
                                   'p2sh_addr',
                                   'redeem_script'])


def get_key(node):
    """Generate a fresh key on node

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    addr = node.getnewaddress()
    pubkey = node.getaddressinfo(addr)['pubkey']
    pkh = hash160(hex_str_to_bytes(pubkey))
    return Key(privkey=node.dumpprivkey(addr),
               pubkey=pubkey,
               p2pkh_script=CScript(
                   [OP_DUP, OP_HASH160, pkh, OP_EQUALVERIFY, OP_CHECKSIG]).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey))


def get_generate_key():
    """Generate a fresh key

    Returns a named tuple of privkey, pubkey and all address and scripts."""
    eckey = ECKey()
    eckey.generate()
    privkey = bytes_to_wif(eckey.get_bytes())
    pubkey = eckey.get_pubkey().get_bytes().hex()
    pkh = hash160(hex_str_to_bytes(pubkey))
    return Key(privkey=privkey,
               pubkey=pubkey,
               p2pkh_script=CScript(
                   [OP_DUP, OP_HASH160, pkh, OP_EQUALVERIFY, OP_CHECKSIG]).hex(),
               p2pkh_addr=key_to_p2pkh(pubkey))


def get_multisig(node):
    """Generate a fresh 2-of-3 multisig on node

    Returns a named tuple of privkeys, pubkeys and all address and scripts."""
    addrs = []
    pubkeys = []
    for _ in range(3):
        addr = node.getaddressinfo(node.getnewaddress())
        addrs.append(addr['address'])
        pubkeys.append(addr['pubkey'])
    script_code = CScript([OP_2] + [hex_str_to_bytes(pubkey)
                                    for pubkey in pubkeys] + [OP_3, OP_CHECKMULTISIG])
    return Multisig(privkeys=[node.dumpprivkey(addr) for addr in addrs],
                    pubkeys=pubkeys,
                    p2sh_script=CScript(
                        [OP_HASH160, hash160(script_code), OP_EQUAL]).hex(),
                    p2sh_addr=script_to_p2sh(script_code),
                    redeem_script=script_code.hex())


def test_address(node, address, **kwargs):
    """Get address info for `address` and test whether the returned values are as expected."""
    addr_info = node.getaddressinfo(address)
    for key, value in kwargs.items():
        if value is None:
            if key in addr_info.keys():
                raise AssertionError(
                    "key {} unexpectedly returned in getaddressinfo.".format(key))
        elif addr_info[key] != value:
            raise AssertionError(
                "key {} value {} did not match expected value {}".format(
                    key, addr_info[key], value))
