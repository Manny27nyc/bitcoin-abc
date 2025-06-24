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

#ifndef BITCOIN_AVALANCHE_PEERMANAGER_H
#define BITCOIN_AVALANCHE_PEERMANAGER_H

#include <avalanche/node.h>
#include <avalanche/orphanproofpool.h>
#include <avalanche/proof.h>
#include <coins.h>
#include <net.h>
#include <pubkey.h>
#include <salteduint256hasher.h>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace avalanche {

/**
 * Maximum number of stakes in the orphanProofs.
 * Benchmarking on a consumer grade computer shows that 10000 stakes can be
 * verified in less than 1 second.
 */
static constexpr size_t AVALANCHE_ORPHANPROOFPOOL_SIZE = 10000;

class Delegation;

struct Slot {
private:
    uint64_t start;
    uint32_t score;
    PeerId peerid;

public:
    Slot(uint64_t startIn, uint32_t scoreIn, PeerId peeridIn)
        : start(startIn), score(scoreIn), peerid(peeridIn) {}

    Slot withStart(uint64_t startIn) const {
        return Slot(startIn, score, peerid);
    }
    Slot withScore(uint64_t scoreIn) const {
        return Slot(start, scoreIn, peerid);
    }
    Slot withPeerId(PeerId peeridIn) const {
        return Slot(start, score, peeridIn);
    }

    uint64_t getStart() const { return start; }
    uint64_t getStop() const { return start + score; }
    uint32_t getScore() const { return score; }
    PeerId getPeerId() const { return peerid; }

    bool contains(uint64_t slot) const {
        return getStart() <= slot && slot < getStop();
    }
    bool precedes(uint64_t slot) const { return slot >= getStop(); }
    bool follows(uint64_t slot) const { return getStart() > slot; }
};

struct Peer {
    PeerId peerid;
    uint32_t index = -1;
    uint32_t node_count = 0;

    std::shared_ptr<Proof> proof;

    using Timestamp = std::chrono::time_point<std::chrono::system_clock>;
    Timestamp time;

    Peer(PeerId peerid_, std::shared_ptr<Proof> proof_)
        : peerid(peerid_), proof(std::move(proof_)),
          time(std::chrono::seconds(GetTime())) {}

    const ProofId &getProofId() const { return proof->getId(); }
    uint32_t getScore() const { return proof->getScore(); }
};

struct proof_index {
    using result_type = ProofId;
    result_type operator()(const Peer &p) const { return p.proof->getId(); }
};

struct next_request_time {};

class PeerManager {
    std::vector<Slot> slots;
    uint64_t slotCount = 0;
    uint64_t fragmentation = 0;

    OrphanProofPool orphanProofs{AVALANCHE_ORPHANPROOFPOOL_SIZE};

    /**
     * Track proof ids to broadcast
     */
    std::unordered_set<ProofId, SaltedProofIdHasher> m_unbroadcast_proofids;

    /**
     * Several nodes can make an avalanche peer. In this case, all nodes are
     * considered interchangeable parts of the same peer.
     */
    using PeerSet = boost::multi_index_container<
        Peer, boost::multi_index::indexed_by<
                  // index by peerid
                  boost::multi_index::hashed_unique<
                      boost::multi_index::member<Peer, PeerId, &Peer::peerid>>,
                  // index by proof
                  boost::multi_index::hashed_unique<
                      boost::multi_index::tag<proof_index>, proof_index,
                      SaltedProofIdHasher>>>;

    PeerId nextPeerId = 0;
    PeerSet peers;

    std::unordered_map<COutPoint, PeerId, SaltedOutpointHasher> utxos;

    using NodeSet = boost::multi_index_container<
        Node,
        boost::multi_index::indexed_by<
            // index by nodeid
            boost::multi_index::hashed_unique<
                boost::multi_index::member<Node, NodeId, &Node::nodeid>>,
            // sorted by peerid/nextRequestTime
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<next_request_time>,
                boost::multi_index::composite_key<
                    Node,
                    boost::multi_index::member<Node, PeerId, &Node::peerid>,
                    boost::multi_index::member<Node, TimePoint,
                                               &Node::nextRequestTime>>>>>;

    NodeSet nodes;

    static constexpr int SELECT_PEER_MAX_RETRY = 3;
    static constexpr int SELECT_NODE_MAX_RETRY = 3;

public:
    /**
     * Node API.
     */
    bool addNode(NodeId nodeid, const Delegation &delegation);
    bool removeNode(NodeId nodeid);

    bool forNode(NodeId nodeid, std::function<bool(const Node &n)> func) const;
    bool updateNextRequestTime(NodeId nodeid, TimePoint timeout);

    /**
     * Randomly select a node to poll.
     */
    NodeId selectNode();

    /**
     * Update the peer set when a new block is connected.
     */
    void updatedBlockTip();

    /****************************************************
     * Functions which are public for testing purposes. *
     ****************************************************/
    /**
     * Provide the PeerId associated with the given proof. If the peer does not
     * exist, then it is created.
     */
    PeerId getPeerId(const std::shared_ptr<Proof> &proof);

    /**
     * Remove an existing peer.
     */
    bool removePeer(const PeerId peerid);

    /**
     * Randomly select a peer to poll.
     */
    PeerId selectPeer() const;

    /**
     * Trigger maintenance of internal data structures.
     * Returns how much slot space was saved after compaction.
     */
    uint64_t compact();

    /**
     * Perform consistency check on internal data structures.
     */
    bool verify() const;

    // Accessors.
    uint64_t getSlotCount() const { return slotCount; }
    uint64_t getFragmentation() const { return fragmentation; }

    std::vector<Peer> getPeers() const;
    std::vector<NodeId> getNodeIdsForPeer(PeerId peerId) const;

    std::shared_ptr<Proof> getProof(const ProofId &proofid) const;
    Peer::Timestamp getProofTime(const ProofId &proofid) const;

    bool isOrphan(const ProofId &id) const;
    std::shared_ptr<Proof> getOrphan(const ProofId &id) const;

    void addUnbroadcastProof(const ProofId &proofid);
    void removeUnbroadcastProof(const ProofId &proofid);
    void broadcastProofs(const CConnman &connman);

private:
    PeerSet::iterator fetchOrCreatePeer(const std::shared_ptr<Proof> &proof);
    bool addOrUpdateNode(const PeerSet::iterator &it, NodeId nodeid,
                         CPubKey pubkey);
    bool addNodeToPeer(const PeerSet::iterator &it);
    bool removeNodeFromPeer(const PeerSet::iterator &it, uint32_t count = 1);
};

/**
 * This is an internal method that is exposed for testing purposes.
 */
PeerId selectPeerImpl(const std::vector<Slot> &slots, const uint64_t slot,
                      const uint64_t max);

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_PEERMANAGER_H
