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
# Copyright (c) 2014-2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Script to generate list of seed nodes for chainparams.cpp.

This script expects two text files in the directory that is passed as an
argument:

    nodes_main.txt
    nodes_test.txt

These files must consist of lines in the format

    <ip>
    <ip>:<port>
    [<ipv6>]
    [<ipv6>]:<port>
    <onion>.onion
    0xDDBBCCAA (IPv4 little-endian old pnSeeds format)

The output will be two data structures with the peers in binary format:

   static SeedSpec6 pnSeed6_main[]={
   ...
   }
   static SeedSpec6 pnSeed6_test[]={
   ...
   }

These should be pasted into `src/chainparamsseeds.h`.
'''

from base64 import b32decode
from binascii import a2b_hex
import sys
import os
import re

# ipv4 in ipv6 prefix
pchIPv4 = bytearray([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff])
# tor-specific ipv6 prefix
pchOnionCat = bytearray([0xFD, 0x87, 0xD8, 0x7E, 0xEB, 0x43])


def name_to_ipv6(addr):
    if len(addr) > 6 and addr.endswith('.onion'):
        vchAddr = b32decode(addr[0:-6], True)
        if len(vchAddr) != 16 - len(pchOnionCat):
            raise ValueError('Invalid onion {}'.format(vchAddr))
        return pchOnionCat + vchAddr
    elif '.' in addr:  # IPv4
        return pchIPv4 + bytearray((int(x) for x in addr.split('.')))
    elif ':' in addr:  # IPv6
        sub = [[], []]  # prefix, suffix
        x = 0
        addr = addr.split(':')
        for i, comp in enumerate(addr):
            if comp == '':
                # skip empty component at beginning or end
                if i == 0 or i == (len(addr) - 1):
                    continue
                x += 1  # :: skips to suffix
                assert(x < 2)
            else:  # two bytes per component
                val = int(comp, 16)
                sub[x].append(val >> 8)
                sub[x].append(val & 0xff)
        nullbytes = 16 - len(sub[0]) - len(sub[1])
        assert((x == 0 and nullbytes == 0) or (x == 1 and nullbytes > 0))
        return bytearray(sub[0] + ([0] * nullbytes) + sub[1])
    elif addr.startswith('0x'):  # IPv4-in-little-endian
        return pchIPv4 + bytearray(reversed(a2b_hex(addr[2:])))
    else:
        raise ValueError('Could not parse address {}'.format(addr))


def parse_spec(s, defaultport):
    match = re.match(r'\[([0-9a-fA-F:]+)\](?::([0-9]+))?$', s)
    if match:  # ipv6
        host = match.group(1)
        port = match.group(2)
    elif s.count(':') > 1:  # ipv6, no port
        host = s
        port = ''
    else:
        (host, _, port) = s.partition(':')

    if not port:
        port = defaultport
    else:
        port = int(port)

    host = name_to_ipv6(host)

    return (host, port)


def process_nodes(g, f, structname, defaultport):
    g.write('static SeedSpec6 {}[] = {{\n'.format(structname))
    first = True
    for line in f:
        comment = line.find('#')
        if comment != -1:
            line = line[0:comment]
        line = line.strip()
        if not line:
            continue
        if not first:
            g.write(',\n')
        first = False

        (host, port) = parse_spec(line, defaultport)
        hoststr = ','.join(('0x{:02x}'.format(b)) for b in host)
        g.write('    {{{{{}}}, {}}}'.format(hoststr, port))
    g.write('\n};\n')


def main():
    if len(sys.argv) < 2:
        print(
            'Usage: {} <path_to_nodes_txt>'.format(
                sys.argv[0]),
            file=sys.stderr)
        sys.exit(1)
    g = sys.stdout
    indir = sys.argv[1]
    g.write('#ifndef BITCOIN_CHAINPARAMSSEEDS_H\n')
    g.write('#define BITCOIN_CHAINPARAMSSEEDS_H\n')
    g.write('/**\n')
    g.write(' * List of fixed seed nodes for the bitcoin network\n')
    g.write(' * @{} by contrib/seeds/generate-seeds.py\n'.format('generated'))
    g.write(' *\n')
    g.write(' * Each line contains a 16-byte IPv6 address and a port.\n')
    g.write(
        ' * IPv4 as well as onion addresses are wrapped inside an IPv6 address accordingly.\n')
    g.write(' */\n')
    with open(os.path.join(indir, 'nodes_main.txt'), 'r', encoding="utf8") as f:
        process_nodes(g, f, 'pnSeed6_main', 8333)
    g.write('\n')
    with open(os.path.join(indir, 'nodes_test.txt'), 'r', encoding="utf8") as f:
        process_nodes(g, f, 'pnSeed6_test', 18333)
    g.write('#endif // BITCOIN_CHAINPARAMSSEEDS_H\n')


if __name__ == '__main__':
    main()
