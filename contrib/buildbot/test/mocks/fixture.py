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
#
# Copyright (c) 2019-2020 The Bitcoin ABC developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from flask.json import JSONEncoder

# Dummy values to be specified in tests


class MockData:
    pass

# TODO: When Python3.7 becomes the minimum version, remove MockJSONEncoder and
# MockData base class.  Decorate data classes with @dataclass from package
# 'dataclasses' instead.


class MockJSONEncoder(JSONEncoder):
    def default(self, o):
        if isinstance(o, MockData):
            return o.__dict__
        return super(self).default(o)
