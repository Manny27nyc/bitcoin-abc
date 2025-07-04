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
# Copyright (c) 2019 The Bitcoin ABC developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import mock
import unittest

from slackbot import SlackBot

import test.mocks.slackbot


def mockSlackBot():
    channels = {
        'test': '#test-channel',
    }
    slackbot = SlackBot(mock.Mock, 'slack-token', channels)
    return slackbot


class SlackbotTestCase(unittest.TestCase):
    def setUp(self):
        self.slackbot = mockSlackBot()

    def tearDown(self):
        pass

    def test_postMessage(self):
        message = "test message"
        expectedAssertionMessage = "Invalid channel: Channel must be a user ID or configured with a channel name"
        self.assertRaisesRegex(
            AssertionError,
            expectedAssertionMessage,
            self.slackbot.postMessage,
            None,
            message)
        self.assertRaisesRegex(
            AssertionError,
            expectedAssertionMessage,
            self.slackbot.postMessage,
            'doesnt-exist',
            message)

        self.slackbot.postMessage('U1234', message)
        self.slackbot.client.chat_postMessage.assert_called_with(
            channel='U1234', text=message)

        self.slackbot.postMessage('test', message)
        self.slackbot.client.chat_postMessage.assert_called_with(
            channel='#test-channel', text=message)

    def test_getUserByName(self):
        user = test.mocks.slackbot.user()
        self.slackbot.client.users_list.return_value = test.mocks.slackbot.users_list(
            initialUsers=[user])
        self.assertIsNone(self.slackbot.getUserByName('Other Name'))
        self.assertEqual(self.slackbot.getUserByName('Real Name'), user)
        self.assertEqual(self.slackbot.getUserByName(
            'Real Name Normalized'), user)
        self.assertEqual(self.slackbot.getUserByName('Display Name'), user)
        self.assertEqual(self.slackbot.getUserByName(
            'Display Name Normalized'), user)

    def test_formatMentionByName(self):
        user = test.mocks.slackbot.user()
        expectedMention = '<@{}>'.format(user['id'])
        self.slackbot.client.users_list.return_value = test.mocks.slackbot.users_list(
            initialUsers=[user])
        self.assertIsNone(self.slackbot.formatMentionByName('testname'))
        self.assertEqual(
            self.slackbot.formatMentionByName('Real Name'),
            expectedMention)
        self.assertEqual(self.slackbot.formatMentionByName(
            'Real Name Normalized'), expectedMention)
        self.assertEqual(
            self.slackbot.formatMentionByName('Display Name'),
            expectedMention)
        self.assertEqual(self.slackbot.formatMentionByName(
            'Display Name Normalized'), expectedMention)


if __name__ == '__main__':
    unittest.main()
