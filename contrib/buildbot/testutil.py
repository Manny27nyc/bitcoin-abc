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

from pprint import pformat


def AnyWith(cls, attrs=None):
    class AnyWith(cls):
        def __eq__(self, other):
            if not isinstance(other, cls):
                raise AssertionError("Argument class type did not match.\nExpected:\n{}\n\nActual:\n{}".format(
                    pformat(cls), pformat(other)))
            if attrs is not None:
                for attr, expectedValue in attrs.items():
                    if not hasattr(other, attr):
                        raise AssertionError("Argument missing expected attribute:\n{}\n\nArgument has:\n{}".format(
                            pformat(attr), pformat(dir(other))))
                    actualValue = getattr(other, attr)
                    if not isinstance(expectedValue, type(actualValue)):
                        raise AssertionError(
                            "Argument attribute type did not match.\nExpected:\n{}\n\nActual:\n{}\nFor expected value:\n{}".format(
                                type(expectedValue).__name__,
                                type(actualValue).__name__,
                                pformat(expectedValue)))
                    if expectedValue != actualValue:
                        raise AssertionError("Argument attribute value did not match.\nExpected:\n{}\n\nActual:\n{}".format(
                            pformat(expectedValue), pformat(actualValue)))
            return True
    return AnyWith()
