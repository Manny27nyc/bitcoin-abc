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
# Copyright (c) 2017-2019 The Bitcoin ABC developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


import sys
import os
import argparse
import logging
import slack

from logging.handlers import RotatingFileHandler

from cirrus import Cirrus
from phabricator_wrapper import PhabWrapper
from slackbot import SlackBot
from teamcity_wrapper import TeamCity

import server

# Setup global parameters
conduit_token = os.getenv("TEAMCITY_CONDUIT_TOKEN", None)
db_file_no_ext = os.getenv("DATABASE_FILE_NO_EXT", None)
tc_user = os.getenv("TEAMCITY_USERNAME", None)
tc_pass = os.getenv("TEAMCITY_PASSWORD", None)
phabricatorUrl = os.getenv(
    "PHABRICATOR_URL", "https://reviews.bitcoinabc.org/api/")
slack_token = os.getenv('SLACK_BOT_TOKEN', None)

tc = TeamCity('https://build.bitcoinabc.org', tc_user, tc_pass)
phab = PhabWrapper(host=phabricatorUrl, token=conduit_token)
phab.update_interfaces()
slack_channels = {
    #  #dev
    'dev': 'C62NSDC6N',
    #  #abcbot-testing
    'test': 'CQMSVCY66',
    #  #infra-support
    'infra': 'G016CFAV8KS',
}
slackbot = SlackBot(slack.WebClient, slack_token, slack_channels)
cirrus = Cirrus()


def main(args):
    parser = argparse.ArgumentParser(
        description='Continuous integration build bot service.')
    parser.add_argument(
        '-p', '--port', help='port for server to start', type=int, default=8080)
    parser.add_argument(
        '-l', '--log-file', help='log file to dump requests payload', type=str, default='log.log')
    args = parser.parse_args()
    port = args.port
    log_file = args.log_file

    app = server.create_server(
        tc,
        phab,
        slackbot,
        cirrus,
        db_file_no_ext=db_file_no_ext)

    formater = logging.Formatter(
        '[%(asctime)s] %(levelname)s in %(module)s: %(message)s')
    fileHandler = RotatingFileHandler(log_file, maxBytes=10000, backupCount=1)
    fileHandler.setFormatter(formater)
    app.logger.addHandler(fileHandler)

    app.run(host="0.0.0.0", port=port)


if __name__ == "__main__":
    main(sys.argv)
