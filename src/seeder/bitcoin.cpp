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
// Copyright (c) 2017-2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <seeder/bitcoin.h>

#include <chainparams.h>
#include <hash.h>
#include <netbase.h>
#include <primitives/blockhash.h>
#include <seeder/db.h>
#include <seeder/messagewriter.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>

#include <algorithm>

#define BITCOIN_SEED_NONCE 0x0539a019ca550825ULL

void CSeederNode::Send() {
    if (sock == INVALID_SOCKET) {
        return;
    }
    if (vSend.empty()) {
        return;
    }
    int nBytes = send(sock, &vSend[0], vSend.size(), 0);
    if (nBytes > 0) {
        vSend.erase(vSend.begin(), vSend.begin() + nBytes);
    } else {
        close(sock);
        sock = INVALID_SOCKET;
    }
}

PeerMessagingState CSeederNode::ProcessMessage(std::string strCommand,
                                               CDataStream &recv) {
    // tfm::format(std::cout, "%s: RECV %s\n", ToString(you),
    // strCommand);
    if (strCommand == NetMsgType::VERSION) {
        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        uint64_t nServiceInt;
        recv >> nVersion >> nServiceInt >> nTime >> addrMe;
        you.nServices = ServiceFlags(nServiceInt);
        recv >> addrFrom >> nNonce;
        recv >> strSubVer;
        recv >> nStartingHeight;

        vSend.SetVersion(std::min(nVersion, PROTOCOL_VERSION));
        MessageWriter::WriteMessage(vSend, NetMsgType::VERACK);
        return PeerMessagingState::AwaitingMessages;
    }

    if (strCommand == NetMsgType::VERACK) {
        vRecv.SetVersion(std::min(nVersion, PROTOCOL_VERSION));
        // tfm::format(std::cout, "\n%s: version %i\n", ToString(you),
        // nVersion);
        if (vAddr) {
            MessageWriter::WriteMessage(vSend, NetMsgType::GETADDR);
            std::vector<BlockHash> locatorHash(
                1, Params().Checkpoints().mapCheckpoints.rbegin()->second);
            MessageWriter::WriteMessage(vSend, NetMsgType::GETHEADERS,
                                        CBlockLocator(locatorHash), uint256());
            doneAfter = GetTime() + GetTimeout();
        } else {
            doneAfter = GetTime() + 1;
        }
        return PeerMessagingState::AwaitingMessages;
    }

    if (strCommand == NetMsgType::ADDR && vAddr) {
        std::vector<CAddress> vAddrNew;
        recv >> vAddrNew;
        // tfm::format(std::cout, "%s: got %i addresses\n",
        // ToString(you),
        //        (int)vAddrNew.size());
        int64_t now = GetTime();
        std::vector<CAddress>::iterator it = vAddrNew.begin();
        if (vAddrNew.size() > 1) {
            if (doneAfter == 0 || doneAfter > now + 1) {
                doneAfter = now + 1;
            }
        }
        while (it != vAddrNew.end()) {
            CAddress &addr = *it;
            // tfm::format(std::cout, "%s: got address %s\n",
            // ToString(you),
            //        addr.ToString(), (int)(vAddr->size()));
            it++;
            if (addr.nTime <= 100000000 || addr.nTime > now + 600) {
                addr.nTime = now - 5 * 86400;
            }
            if (addr.nTime > now - 604800) {
                vAddr->push_back(addr);
            }
            // tfm::format(std::cout, "%s: added address %s (#%i)\n",
            // ToString(you),
            //        addr.ToString(), (int)(vAddr->size()));
            if (vAddr->size() > ADDR_SOFT_CAP) {
                doneAfter = 1;
                return PeerMessagingState::Finished;
            }
        }
        return PeerMessagingState::AwaitingMessages;
    }

    return PeerMessagingState::AwaitingMessages;
}

bool CSeederNode::ProcessMessages() {
    if (vRecv.empty()) {
        return false;
    }

    const CMessageHeader::MessageMagic netMagic = Params().NetMagic();

    do {
        CDataStream::iterator pstart = std::search(
            vRecv.begin(), vRecv.end(), BEGIN(netMagic), END(netMagic));
        uint32_t nHeaderSize =
            GetSerializeSize(CMessageHeader(netMagic), vRecv.GetVersion());
        if (vRecv.end() - pstart < nHeaderSize) {
            if (vRecv.size() > nHeaderSize) {
                vRecv.erase(vRecv.begin(), vRecv.end() - nHeaderSize);
            }
            break;
        }
        vRecv.erase(vRecv.begin(), pstart);
        std::vector<char> vHeaderSave(vRecv.begin(),
                                      vRecv.begin() + nHeaderSize);
        CMessageHeader hdr(netMagic);
        vRecv >> hdr;
        if (!hdr.IsValidWithoutConfig(netMagic)) {
            // tfm::format(std::cout, "%s: BAD (invalid header)\n",
            // ToString(you));
            ban = 100000;
            return true;
        }
        std::string strCommand = hdr.GetCommand();
        unsigned int nMessageSize = hdr.nMessageSize;
        if (nMessageSize > MAX_SIZE) {
            // tfm::format(std::cout, "%s: BAD (message too large)\n",
            // ToString(you));
            ban = 100000;
            return true;
        }
        if (nMessageSize > vRecv.size()) {
            vRecv.insert(vRecv.begin(), vHeaderSave.begin(), vHeaderSave.end());
            break;
        }
        if (vRecv.GetVersion() >= 209) {
            uint256 hash = Hash(MakeSpan(vRecv).first(nMessageSize));
            if (memcmp(hash.begin(), hdr.pchChecksum,
                       CMessageHeader::CHECKSUM_SIZE) != 0) {
                continue;
            }
        }
        CDataStream vMsg(vRecv.begin(), vRecv.begin() + nMessageSize,
                         vRecv.GetType(), vRecv.GetVersion());
        vRecv.ignore(nMessageSize);
        if (ProcessMessage(strCommand, vMsg) == PeerMessagingState::Finished) {
            return true;
        }
        // tfm::format(std::cout, "%s: done processing %s\n",
        // ToString(you),
        //        strCommand);
    } while (1);
    return false;
}

CSeederNode::CSeederNode(const CService &ip, std::vector<CAddress> *vAddrIn)
    : sock(INVALID_SOCKET), vSend(SER_NETWORK, 0), vRecv(SER_NETWORK, 0),
      nHeaderStart(-1), nMessageStart(-1), nVersion(0), vAddr(vAddrIn), ban(0),
      doneAfter(0), you(ip, ServiceFlags(NODE_NETWORK)) {
    if (GetTime() > 1329696000) {
        vSend.SetVersion(209);
        vRecv.SetVersion(209);
    }
}

bool CSeederNode::Run() {
    // FIXME: This logic is duplicated with CConnman::ConnectNode for no
    // good reason.
    bool connected = false;
    proxyType proxy;

    if (you.IsValid()) {
        bool proxyConnectionFailed = false;

        if (GetProxy(you.GetNetwork(), proxy)) {
            sock = CreateSocket(proxy.proxy);
            if (sock == INVALID_SOCKET) {
                return false;
            }
            connected = ConnectThroughProxy(
                proxy, you.ToStringIP(), you.GetPort(), sock, nConnectTimeout,
                proxyConnectionFailed);
        } else {
            // no proxy needed (none set for target network)
            sock = CreateSocket(you);
            if (sock == INVALID_SOCKET) {
                return false;
            }
            // no proxy needed (none set for target network)
            connected =
                ConnectSocketDirectly(you, sock, nConnectTimeout, false);
        }
    }

    if (!connected) {
        // tfm::format(std::cout, "Cannot connect to %s\n",
        // ToString(you));
        CloseSocket(sock);
        return false;
    }

    // Write version message
    uint64_t nLocalServices = 0;
    uint64_t nLocalNonce = BITCOIN_SEED_NONCE;
    CService myService;
    CAddress me(myService, ServiceFlags(NODE_NETWORK));
    std::string ver = "/bitcoin-cash-seeder:0.15/";
    MessageWriter::WriteMessage(vSend, NetMsgType::VERSION, PROTOCOL_VERSION,
                                nLocalServices, GetTime(), you, me, nLocalNonce,
                                ver, GetRequireHeight());
    Send();

    bool res = true;
    int64_t now;
    while (now = GetTime(), ban == 0 && (doneAfter == 0 || doneAfter > now) &&
                                sock != INVALID_SOCKET) {
        char pchBuf[0x10000];
        fd_set fdsetRecv;
        fd_set fdsetError;
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetError);
        FD_SET(sock, &fdsetRecv);
        FD_SET(sock, &fdsetError);
        struct timeval wa;
        if (doneAfter) {
            wa.tv_sec = doneAfter - now;
            wa.tv_usec = 0;
        } else {
            wa.tv_sec = GetTimeout();
            wa.tv_usec = 0;
        }
        int ret = select(sock + 1, &fdsetRecv, nullptr, &fdsetError, &wa);
        if (ret != 1) {
            if (!doneAfter) {
                res = false;
            }
            break;
        }
        int nBytes = recv(sock, pchBuf, sizeof(pchBuf), 0);
        int nPos = vRecv.size();
        if (nBytes > 0) {
            vRecv.resize(nPos + nBytes);
            memcpy(&vRecv[nPos], pchBuf, nBytes);
        } else if (nBytes == 0) {
            // tfm::format(std::cout, "%s: BAD (connection closed
            // prematurely)\n",
            //        ToString(you));
            res = false;
            break;
        } else {
            // tfm::format(std::cout, "%s: BAD (connection error)\n",
            // ToString(you));
            res = false;
            break;
        }
        ProcessMessages();
        Send();
    }
    if (sock == INVALID_SOCKET) {
        res = false;
    }
    close(sock);
    sock = INVALID_SOCKET;
    return (ban == 0) && res;
}
