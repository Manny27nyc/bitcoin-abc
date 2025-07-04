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
# Copyright (c) 2012-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Extract _("...") strings for translation and convert to Qt stringdefs so that
they can be picked up by Qt linguist.
'''
from subprocess import Popen, PIPE
import operator
import os
import sys

OUT_CPP = "qt/bitcoinstrings.cpp"
EMPTY = ['""']


def parse_po(text):
    """
    Parse 'po' format produced by xgettext.
    Return a list of (msgid,msgstr) tuples.
    """
    messages = []
    msgid = []
    msgstr = []
    in_msgid = False
    in_msgstr = False

    for line in text.split('\n'):
        line = line.rstrip('\r')
        if line.startswith('msgid '):
            if in_msgstr:
                messages.append((msgid, msgstr))
                in_msgstr = False
            # message start
            in_msgid = True

            msgid = [line[6:]]
        elif line.startswith('msgstr '):
            in_msgid = False
            in_msgstr = True
            msgstr = [line[7:]]
        elif line.startswith('"'):
            if in_msgid:
                msgid.append(line)
            if in_msgstr:
                msgstr.append(line)

    if in_msgstr:
        messages.append((msgid, msgstr))

    return messages


files = sys.argv[1:]

# xgettext -n --keyword=_ $FILES
XGETTEXT = os.getenv('XGETTEXT', 'xgettext')
if not XGETTEXT:
    print(
        'Cannot extract strings: xgettext utility is not installed or not configured.',
        file=sys.stderr)
    print('Please install package "gettext" and re-run \'./configure\'.',
          file=sys.stderr)
    sys.exit(1)
child = Popen([XGETTEXT, '--output=-', '-n',
               '--keyword=_'] + files, stdout=PIPE)
(out, err) = child.communicate()

messages = parse_po(out.decode('utf-8'))

f = open(OUT_CPP, 'w', encoding="utf8")
f.write('#include <QtGlobal>\n')
f.write('// Automatically @{} by extract_strings_qt.py\n'.format('generated'))
f.write("""
#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif
""")
f.write('static const char UNUSED *bitcoin_strings[] = {\n')
f.write('QT_TRANSLATE_NOOP("bitcoin-abc", "{}"),\n'.format(os.getenv('COPYRIGHT_HOLDERS'),))
messages.sort(key=operator.itemgetter(0))
for (msgid, msgstr) in messages:
    if msgid != EMPTY:
        f.write('QT_TRANSLATE_NOOP("bitcoin-abc", {}),\n'.format('\n'.join(msgid)))
f.write('};\n')
f.close()
