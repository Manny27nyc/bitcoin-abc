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
# Copyright (c) 2020 The Bitcoin ABC developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from urllib.parse import (
    quote,
    unquote,
    urlencode,
    urlsplit,
    urlunsplit
)


class Badge:
    def __init__(self, **kwargs):
        self.base_url = 'https://img.shields.io/static/v1'

        # Provide some defaults, potentially updated by kwargs
        self.query = {
            'label': 'shieldio',
            'message': 'unknown',
            'color': 'inactive',
        }
        self.query.update(kwargs)

    def get_badge_url(self, **kwargs):
        scheme, netloc, path = urlsplit(self.base_url)[0:3]
        return urlunsplit((
            scheme,
            netloc,
            path,
            unquote(urlencode({**self.query, **kwargs},
                              doseq=True, quote_via=quote)),
            ''
        ))


class RasterBadge(Badge):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.base_url = 'https://raster.shields.io/static/v1'
