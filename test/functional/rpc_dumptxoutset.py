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
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

import hashlib
from pathlib import Path


class DumptxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)
        node.generate(100)

        FILENAME = 'txoutset.dat'
        out = node.dumptxoutset(FILENAME)
        expected_path = Path(node.datadir) / self.chain / FILENAME

        assert expected_path.is_file()

        assert_equal(out['coins_written'], 100)
        assert_equal(out['base_height'], 100)
        assert_equal(out['path'], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out['base_hash'],
            '65d0aec2439aae14373c153f596fb90a87b643d9bff3e65f250aa8f055e6816b')

        with open(str(expected_path), 'rb') as f:
            digest = hashlib.sha256(f.read()).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(
                digest,
                '05957e146e38153d84e9294999cc24f0dcdb9902c4834b32c79ae8e8985babea')

        # Specifying a path to an existing file will fail.
        assert_raises_rpc_error(
            -8, '{} already exists'.format(FILENAME), node.dumptxoutset, FILENAME)


if __name__ == '__main__':
    DumptxoutsetTest().main()
