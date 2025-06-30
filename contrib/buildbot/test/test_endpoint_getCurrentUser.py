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
# Copyright (c) 2017-2020 The Bitcoin ABC developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test.abcbot_fixture import ABCBotFixture, TEST_USER
import unittest


class EndpointGetCurrentUserTestCase(ABCBotFixture):
    def test_currentUser(self):
        rv = self.app.get('/getCurrentUser', headers=self.headers)
        self.assertEqual(rv.data, TEST_USER.encode())


if __name__ == '__main__':
    unittest.main()
