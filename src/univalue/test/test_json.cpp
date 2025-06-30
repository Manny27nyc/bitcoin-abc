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
// Test program that can be called by the JSON test suite at
// https://github.com/nst/JSONTestSuite.
//
// It reads JSON input from stdin and exits with code 0 if it can be parsed
// successfully. It also pretty prints the parsed JSON value to stdout.

#include <iostream>
#include <string>
#include "univalue.h"

using namespace std;

int main (int argc, char *argv[])
{
    UniValue val;
    if (val.read(string(istreambuf_iterator<char>(cin),
                        istreambuf_iterator<char>()))) {
        cout << val.write(1 /* prettyIndent */, 4 /* indentLevel */) << endl;
        return 0;
    } else {
        cerr << "JSON Parse Error." << endl;
        return 1;
    }
}
