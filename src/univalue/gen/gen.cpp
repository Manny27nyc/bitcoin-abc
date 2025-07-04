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
// Copyright 2014 BitPay Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

//
// To re-create univalue_escapes.h:
// $ g++ -o gen gen.cpp
// $ ./gen > univalue_escapes.h
//

#include <stdio.h>
#include <string.h>
#include "univalue.h"

static bool initEscapes;
static std::string escapes[256];

static void initJsonEscape()
{
    // Escape all lower control characters (some get overridden with smaller sequences below)
    for (int ch=0x00; ch<0x20; ++ch) {
        char tmpbuf[20];
        snprintf(tmpbuf, sizeof(tmpbuf), "\\u%04x", ch);
        escapes[ch] = std::string(tmpbuf);
    }

    escapes[(int)'"'] = "\\\"";
    escapes[(int)'\\'] = "\\\\";
    escapes[(int)'\b'] = "\\b";
    escapes[(int)'\f'] = "\\f";
    escapes[(int)'\n'] = "\\n";
    escapes[(int)'\r'] = "\\r";
    escapes[(int)'\t'] = "\\t";
    escapes[(int)'\x7f'] = "\\u007f"; // U+007F DELETE

    initEscapes = true;
}

static void outputEscape()
{
	printf(	"// Automatically generated file. Do not modify.\n"
		"#ifndef BITCOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n"
		"#define BITCOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n"
		"static const char *escapes[256] = {\n");

	for (unsigned int i = 0; i < 256; i++) {
		if (escapes[i].empty()) {
			printf("\tNULL,\n");
		} else {
			printf("\t\"");

			unsigned int si;
			for (si = 0; si < escapes[i].size(); si++) {
				char ch = escapes[i][si];
				switch (ch) {
				case '"':
					printf("\\\"");
					break;
				case '\\':
					printf("\\\\");
					break;
				default:
					printf("%c", escapes[i][si]);
					break;
				}
			}

			printf("\",\n");
		}
	}

	printf(	"};\n"
		"#endif // BITCOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n");
}

int main (int argc, char *argv[])
{
	initJsonEscape();
	outputEscape();
	return 0;
}

