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
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __cplusplus
#error This header can only be compiled as C++.
#endif

#ifndef BITCOIN_PROTOCOL_H
#define BITCOIN_PROTOCOL_H

#include <netaddress.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>

#include <array>
#include <cstdint>
#include <string>

class Config;

/**
 * Maximum length of incoming protocol messages (Currently 2MB).
 * NB: Messages propagating block content are not subject to this limit.
 */
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 2 * 1024 * 1024;

/**
 * Message header.
 * (4) message start.
 * (12) command.
 * (4) size.
 * (4) checksum.
 */
class CMessageHeader {
public:
    static constexpr size_t MESSAGE_START_SIZE = 4;
    static constexpr size_t COMMAND_SIZE = 12;
    static constexpr size_t MESSAGE_SIZE_SIZE = 4;
    static constexpr size_t CHECKSUM_SIZE = 4;
    static constexpr size_t MESSAGE_SIZE_OFFSET =
        MESSAGE_START_SIZE + COMMAND_SIZE;
    static constexpr size_t CHECKSUM_OFFSET =
        MESSAGE_SIZE_OFFSET + MESSAGE_SIZE_SIZE;
    static constexpr size_t HEADER_SIZE =
        MESSAGE_START_SIZE + COMMAND_SIZE + MESSAGE_SIZE_SIZE + CHECKSUM_SIZE;
    typedef std::array<uint8_t, MESSAGE_START_SIZE> MessageMagic;

    explicit CMessageHeader(const MessageMagic &pchMessageStartIn);

    /**
     * Construct a P2P message header from message-start characters, a command
     * and the size of the message.
     * @note Passing in a `pszCommand` longer than COMMAND_SIZE will result in a
     * run-time assertion error.
     */
    CMessageHeader(const MessageMagic &pchMessageStartIn,
                   const char *pszCommand, unsigned int nMessageSizeIn);

    std::string GetCommand() const;
    bool IsValid(const Config &config) const;
    bool IsValidWithoutConfig(const MessageMagic &magic) const;
    bool IsOversized(const Config &config) const;

    SERIALIZE_METHODS(CMessageHeader, obj) {
        READWRITE(obj.pchMessageStart, obj.pchCommand, obj.nMessageSize,
                  obj.pchChecksum);
    }

    MessageMagic pchMessageStart;
    std::array<char, COMMAND_SIZE> pchCommand;
    uint32_t nMessageSize;
    uint8_t pchChecksum[CHECKSUM_SIZE];
};

/**
 * Bitcoin protocol message types. When adding new message types, don't forget
 * to update allNetMessageTypes in protocol.cpp.
 */
namespace NetMsgType {

/**
 * The version message provides information about the transmitting node to the
 * receiving node at the beginning of a connection.
 * @see https://bitcoin.org/en/developer-reference#version
 */
extern const char *VERSION;
/**
 * The verack message acknowledges a previously-received version message,
 * informing the connecting node that it can begin to send other messages.
 * @see https://bitcoin.org/en/developer-reference#verack
 */
extern const char *VERACK;
/**
 * The addr (IP address) message relays connection information for peers on the
 * network.
 * @see https://bitcoin.org/en/developer-reference#addr
 */
extern const char *ADDR;
/**
 * The addrv2 message relays connection information for peers on the network
 * just like the addr message, but is extended to allow gossiping of longer node
 * addresses (see BIP155).
 */
extern const char *ADDRV2;
/**
 * The sendaddrv2 message signals support for receiving ADDRV2 messages
 * (BIP155). It also implies that its sender can encode as ADDRV2 and would send
 * ADDRV2 instead of ADDR to a peer that has signaled ADDRV2 support by sending
 * SENDADDRV2.
 */
extern const char *SENDADDRV2;
/**
 * The inv message (inventory message) transmits one or more inventories of
 * objects known to the transmitting peer.
 * @see https://bitcoin.org/en/developer-reference#inv
 */
extern const char *INV;
/**
 * The getdata message requests one or more data objects from another node.
 * @see https://bitcoin.org/en/developer-reference#getdata
 */
extern const char *GETDATA;
/**
 * The merkleblock message is a reply to a getdata message which requested a
 * block using the inventory type MSG_MERKLEBLOCK.
 * @since protocol version 70001 as described by BIP37.
 * @see https://bitcoin.org/en/developer-reference#merkleblock
 */
extern const char *MERKLEBLOCK;
/**
 * The getblocks message requests an inv message that provides block header
 * hashes starting from a particular point in the block chain.
 * @see https://bitcoin.org/en/developer-reference#getblocks
 */
extern const char *GETBLOCKS;
/**
 * The getheaders message requests a headers message that provides block
 * headers starting from a particular point in the block chain.
 * @since protocol version 31800.
 * @see https://bitcoin.org/en/developer-reference#getheaders
 */
extern const char *GETHEADERS;
/**
 * The tx message transmits a single transaction.
 * @see https://bitcoin.org/en/developer-reference#tx
 */
extern const char *TX;
/**
 * The headers message sends one or more block headers to a node which
 * previously requested certain headers with a getheaders message.
 * @since protocol version 31800.
 * @see https://bitcoin.org/en/developer-reference#headers
 */
extern const char *HEADERS;
/**
 * The block message transmits a single serialized block.
 * @see https://bitcoin.org/en/developer-reference#block
 */
extern const char *BLOCK;
/**
 * The getaddr message requests an addr message from the receiving node,
 * preferably one with lots of IP addresses of other receiving nodes.
 * @see https://bitcoin.org/en/developer-reference#getaddr
 */
extern const char *GETADDR;
/**
 * The mempool message requests the TXIDs of transactions that the receiving
 * node has verified as valid but which have not yet appeared in a block.
 * @since protocol version 60002.
 * @see https://bitcoin.org/en/developer-reference#mempool
 */
extern const char *MEMPOOL;
/**
 * The ping message is sent periodically to help confirm that the receiving
 * peer is still connected.
 * @see https://bitcoin.org/en/developer-reference#ping
 */
extern const char *PING;
/**
 * The pong message replies to a ping message, proving to the pinging node that
 * the ponging node is still alive.
 * @since protocol version 60001 as described by BIP31.
 * @see https://bitcoin.org/en/developer-reference#pong
 */
extern const char *PONG;
/**
 * The notfound message is a reply to a getdata message which requested an
 * object the receiving node does not have available for relay.
 * @since protocol version 70001.
 * @see https://bitcoin.org/en/developer-reference#notfound
 */
extern const char *NOTFOUND;
/**
 * The filterload message tells the receiving peer to filter all relayed
 * transactions and requested merkle blocks through the provided filter.
 * @since protocol version 70001 as described by BIP37.
 *   Only available with service bit NODE_BLOOM since protocol version
 *   70011 as described by BIP111.
 * @see https://bitcoin.org/en/developer-reference#filterload
 */
extern const char *FILTERLOAD;
/**
 * The filteradd message tells the receiving peer to add a single element to a
 * previously-set bloom filter, such as a new public key.
 * @since protocol version 70001 as described by BIP37.
 *   Only available with service bit NODE_BLOOM since protocol version
 *   70011 as described by BIP111.
 * @see https://bitcoin.org/en/developer-reference#filteradd
 */
extern const char *FILTERADD;
/**
 * The filterclear message tells the receiving peer to remove a previously-set
 * bloom filter.
 * @since protocol version 70001 as described by BIP37.
 *   Only available with service bit NODE_BLOOM since protocol version
 *   70011 as described by BIP111.
 * @see https://bitcoin.org/en/developer-reference#filterclear
 */
extern const char *FILTERCLEAR;
/**
 * Indicates that a node prefers to receive new block announcements via a
 * "headers" message rather than an "inv".
 * @since protocol version 70012 as described by BIP130.
 * @see https://bitcoin.org/en/developer-reference#sendheaders
 */
extern const char *SENDHEADERS;
/**
 * The feefilter message tells the receiving peer not to inv us any txs
 * which do not meet the specified min fee rate.
 * @since protocol version 70013 as described by BIP133
 */
extern const char *FEEFILTER;
/**
 * Contains a 1-byte bool and 8-byte LE version number.
 * Indicates that a node is willing to provide blocks via "cmpctblock" messages.
 * May indicate that a node prefers to receive new block announcements via a
 * "cmpctblock" message rather than an "inv", depending on message contents.
 * @since protocol version 70014 as described by BIP 152
 */
extern const char *SENDCMPCT;
/**
 * Contains a CBlockHeaderAndShortTxIDs object - providing a header and
 * list of "short txids".
 * @since protocol version 70014 as described by BIP 152
 */
extern const char *CMPCTBLOCK;
/**
 * Contains a BlockTransactionsRequest
 * Peer should respond with "blocktxn" message.
 * @since protocol version 70014 as described by BIP 152
 */
extern const char *GETBLOCKTXN;
/**
 * Contains a BlockTransactions.
 * Sent in response to a "getblocktxn" message.
 * @since protocol version 70014 as described by BIP 152
 */
extern const char *BLOCKTXN;
/**
 * getcfilters requests compact filters for a range of blocks.
 * Only available with service bit NODE_COMPACT_FILTERS as described by
 * BIP 157 & 158.
 */
extern const char *GETCFILTERS;
/**
 * cfilter is a response to a getcfilters request containing a single compact
 * filter.
 */
extern const char *CFILTER;
/**
 * getcfheaders requests a compact filter header and the filter hashes for a
 * range of blocks, which can then be used to reconstruct the filter headers
 * for those blocks.
 * Only available with service bit NODE_COMPACT_FILTERS as described by
 * BIP 157 & 158.
 */
extern const char *GETCFHEADERS;
/**
 * cfheaders is a response to a getcfheaders request containing a filter header
 * and a vector of filter hashes for each subsequent block in the requested
 * range.
 */
extern const char *CFHEADERS;
/**
 * getcfcheckpt requests evenly spaced compact filter headers, enabling
 * parallelized download and validation of the headers between them.
 * Only available with service bit NODE_COMPACT_FILTERS as described by
 * BIP 157 & 158.
 */
extern const char *GETCFCHECKPT;
/**
 * cfcheckpt is a response to a getcfcheckpt request containing a vector of
 * evenly spaced filter headers for blocks on the requested chain.
 */
extern const char *CFCHECKPT;
/**
 * Contains a delegation and a signature.
 */
extern const char *AVAHELLO;
/**
 * Contains an avalanche::Poll.
 * Peer should respond with "avaresponse" message.
 */
extern const char *AVAPOLL;
/**
 * Contains an avalanche::Response.
 * Sent in response to a "avapoll" message.
 */
extern const char *AVARESPONSE;
/**
 * Contains an avalanche::Proof.
 * Sent in response to a "getdata" message with inventory type
 * MSG_AVA_PROOF.
 */
extern const char *AVAPROOF;

/**
 * Indicate if the message is used to transmit the content of a block.
 * These messages can be significantly larger than usual messages and therefore
 * may need to be processed differently.
 */
bool IsBlockLike(const std::string &strCommand);
}; // namespace NetMsgType

/** Get a vector of all valid message types (see above) */
const std::vector<std::string> &getAllNetMessageTypes();

/**
 * nServices flags.
 */
enum ServiceFlags : uint64_t {
    // NOTE: When adding here, be sure to update serviceFlagToStr too
    // Nothing
    NODE_NONE = 0,
    // NODE_NETWORK means that the node is capable of serving the complete block
    // chain. It is currently set by all Bitcoin ABC non pruned nodes, and is
    // unset by SPV clients or other light clients.
    NODE_NETWORK = (1 << 0),
    // NODE_GETUTXO means the node is capable of responding to the getutxo
    // protocol request. Bitcoin ABC does not support this but a patch set
    // called Bitcoin XT does. See BIP 64 for details on how this is
    // implemented.
    NODE_GETUTXO = (1 << 1),
    // NODE_BLOOM means the node is capable and willing to handle bloom-filtered
    // connections. Bitcoin ABC nodes used to support this by default, without
    // advertising this bit, but no longer do as of protocol version 70011 (=
    // NO_BLOOM_VERSION)
    NODE_BLOOM = (1 << 2),
    // Bit 4 was NODE_XTHIN, removed in v0.22.12

    // Bit 5 was NODE_BITCOIN_CASH, removed in v0.22.8

    // NODE_COMPACT_FILTERS means the node will service basic block filter
    // requests.
    // See BIP157 and BIP158 for details on how this is implemented.
    NODE_COMPACT_FILTERS = (1 << 6),

    // NODE_NETWORK_LIMITED means the same as NODE_NETWORK with the limitation
    // of only serving the last 288 (2 day) blocks
    // See BIP159 for details on how this is implemented.
    NODE_NETWORK_LIMITED = (1 << 10),

    // The last non experimental service bit, helper for looping over the flags
    NODE_LAST_NON_EXPERIMENTAL_SERVICE_BIT = (1 << 23),

    // Bits 24-31 are reserved for temporary experiments. Just pick a bit that
    // isn't getting used, or one not being used much, and notify the
    // bitcoin-development mailing list. Remember that service bits are just
    // unauthenticated advertisements, so your code must be robust against
    // collisions and other cases where nodes may be advertising a service they
    // do not actually support. Other service bits should be allocated via the
    // BIP process.

    // NODE_AVALANCHE means the node supports Bitcoin Cash's avalanche
    // preconsensus mechanism.
    NODE_AVALANCHE = (1 << 24),
};

/**
 * Convert service flags (a bitmask of NODE_*) to human readable strings.
 * It supports unknown service flags which will be returned as "UNKNOWN[...]".
 * @param[in] flags multiple NODE_* bitwise-OR-ed together
 */
std::vector<std::string> serviceFlagsToStr(const uint64_t flags);

/**
 * Gets the set of service flags which are "desirable" for a given peer.
 *
 * These are the flags which are required for a peer to support for them
 * to be "interesting" to us, ie for us to wish to use one of our few
 * outbound connection slots for or for us to wish to prioritize keeping
 * their connection around.
 *
 * Relevant service flags may be peer- and state-specific in that the
 * version of the peer may determine which flags are required (eg in the
 * case of NODE_NETWORK_LIMITED where we seek out NODE_NETWORK peers
 * unless they set NODE_NETWORK_LIMITED and we are out of IBD, in which
 * case NODE_NETWORK_LIMITED suffices).
 *
 * Thus, generally, avoid calling with peerServices == NODE_NONE, unless
 * state-specific flags must absolutely be avoided. When called with
 * peerServices == NODE_NONE, the returned desirable service flags are
 * guaranteed to not change dependent on state - ie they are suitable for
 * use when describing peers which we know to be desirable, but for which
 * we do not have a confirmed set of service flags.
 *
 * If the NODE_NONE return value is changed, contrib/seeds/makeseeds.py
 * should be updated appropriately to filter for the same nodes.
 */
ServiceFlags GetDesirableServiceFlags(ServiceFlags services);

/**
 * Set the current IBD status in order to figure out the desirable service
 * flags
 */
void SetServiceFlagsIBDCache(bool status);

/**
 * A shortcut for (services & GetDesirableServiceFlags(services))
 * == GetDesirableServiceFlags(services), ie determines whether the given
 * set of service flags are sufficient for a peer to be "relevant".
 */
static inline bool HasAllDesirableServiceFlags(ServiceFlags services) {
    return !(GetDesirableServiceFlags(services) & (~services));
}

/**
 * Checks if a peer with the given service flags may be capable of having a
 * robust address-storage DB.
 */
static inline bool MayHaveUsefulAddressDB(ServiceFlags services) {
    return (services & NODE_NETWORK) || (services & NODE_NETWORK_LIMITED);
}

/**
 * A CService with information about it as peer.
 */
class CAddress : public CService {
    static constexpr uint32_t TIME_INIT{100000000};

public:
    CAddress() : CService{} {};
    CAddress(CService ipIn, ServiceFlags nServicesIn)
        : CService{ipIn}, nServices{nServicesIn} {};
    CAddress(CService ipIn, ServiceFlags nServicesIn, uint32_t nTimeIn)
        : CService{ipIn}, nTime{nTimeIn}, nServices{nServicesIn} {};

    void Init();

    SERIALIZE_METHODS(CAddress, obj) {
        SER_READ(obj, obj.nTime = TIME_INIT);
        int nVersion = s.GetVersion();
        if (s.GetType() & SER_DISK) {
            READWRITE(nVersion);
        }
        if ((s.GetType() & SER_DISK) ||
            (nVersion != INIT_PROTO_VERSION && !(s.GetType() & SER_GETHASH))) {
            // The only time we serialize a CAddress object without nTime is in
            // the initial VERSION messages which contain two CAddress records.
            // At that point, the serialization version is INIT_PROTO_VERSION.
            // After the version handshake, serialization version is >=
            // MIN_PEER_PROTO_VERSION and all ADDR messages are serialized with
            // nTime.
            READWRITE(obj.nTime);
        }
        if (nVersion & ADDRV2_FORMAT) {
            uint64_t services_tmp;
            SER_WRITE(obj, services_tmp = obj.nServices);
            READWRITE(Using<CompactSizeFormatter<false>>(services_tmp));
            SER_READ(obj,
                     obj.nServices = static_cast<ServiceFlags>(services_tmp));
        } else {
            READWRITE(Using<CustomUintFormatter<8>>(obj.nServices));
        }
        READWRITEAS(CService, obj);
    }

    // disk and network only
    uint32_t nTime{TIME_INIT};

    ServiceFlags nServices{NODE_NONE};
};

/** getdata message type flags */
const uint32_t MSG_TYPE_MASK = 0xffffffff >> 3;

/**
 * getdata / inv message types.
 * These numbers are defined by the protocol. When adding a new value, be sure
 * to mention it in the respective BIP.
 */
enum GetDataMsg {
    UNDEFINED = 0,
    MSG_TX = 1,
    MSG_BLOCK = 2,
    // The following can only occur in getdata. Invs always use TX or BLOCK.
    //! Defined in BIP37
    MSG_FILTERED_BLOCK = 3,
    //! Defined in BIP152
    MSG_CMPCT_BLOCK = 4,
    MSG_AVA_PROOF = 0x1f000001,
};

/**
 * Inv(ventory) message data.
 * Intended as non-ambiguous identifier of objects (eg. transactions, blocks)
 * held by peers.
 */
class CInv {
public:
    uint32_t type;
    uint256 hash;

    CInv() : type(0), hash() {}
    CInv(uint32_t typeIn, const uint256 &hashIn) : type(typeIn), hash(hashIn) {}

    SERIALIZE_METHODS(CInv, obj) { READWRITE(obj.type, obj.hash); }

    friend bool operator<(const CInv &a, const CInv &b) {
        return a.type < b.type || (a.type == b.type && a.hash < b.hash);
    }

    std::string GetCommand() const;
    std::string ToString() const;

    uint32_t GetKind() const { return type & MSG_TYPE_MASK; }

    bool IsMsgTx() const {
        auto k = GetKind();
        return k == MSG_TX;
    }
    bool IsMsgProof() const {
        auto k = GetKind();
        return k == MSG_AVA_PROOF;
    }
    bool IsMsgBlk() const {
        auto k = GetKind();
        return k == MSG_BLOCK;
    }
    bool IsMsgFilteredBlk() const {
        auto k = GetKind();
        return k == MSG_FILTERED_BLOCK;
    }
    bool IsMsgCmpctBlk() const {
        auto k = GetKind();
        return k == MSG_CMPCT_BLOCK;
    }

    bool IsGenBlkMsg() const {
        auto k = GetKind();
        return k == MSG_BLOCK || k == MSG_FILTERED_BLOCK ||
               k == MSG_CMPCT_BLOCK;
    }
};

#endif // BITCOIN_PROTOCOL_H
