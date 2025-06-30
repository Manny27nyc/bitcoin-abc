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
# Copyright (c) 2019 The Bitcoin developers

import sys


def main(test_name, input_file):
    with open(input_file, "rb") as f:
        contents = f.read()

    print("static unsigned const char {}_raw[] = {{".format(test_name))
    print(", ".join(map(lambda x: "0x{:02x}".format(x), contents)))
    print("};")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("We need additional pylons!")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])
