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
#include <event2/event.h>
#include <cstdint>
#include <iostream>

int main(int argc, char** argv) {
    uint32_t version = event_get_version_number();
    std::cout <<
        ((version & 0xff000000) >> 24) << "." <<
        ((version & 0x00ff0000) >> 16) << "." <<
        ((version & 0x0000ff00) >> 8);
    return 0;
}
