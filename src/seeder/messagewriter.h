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
// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SEEDER_MESSAGEWRITER_H
#define BITCOIN_SEEDER_MESSAGEWRITER_H

#include <config.h>
#include <net.h>
#include <netmessagemaker.h>

namespace MessageWriter {

template <typename... Args>
static void WriteMessage(CDataStream &stream, std::string command,
                         Args &&... args) {
    CSerializedNetMsg payload = CNetMsgMaker(stream.GetVersion())
                                    .Make(command, std::forward<Args>(args)...);
    size_t nMessageSize = payload.data.size();

    // Serialize header
    std::vector<uint8_t> serializedHeader;
    V1TransportSerializer serializer = V1TransportSerializer();
    serializer.prepareForTransport(GetConfig(), payload, serializedHeader);

    // Write message header + payload to outgoing stream
    stream.write(reinterpret_cast<const char *>(serializedHeader.data()),
                 serializedHeader.size());
    if (nMessageSize) {
        stream.write(reinterpret_cast<const char *>(payload.data.data()),
                     nMessageSize);
    }
}

} // namespace MessageWriter

#endif // BITCOIN_SEEDER_MESSAGEWRITER_H
