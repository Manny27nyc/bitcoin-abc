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
// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <avalanche/delegationbuilder.h>
#include <avalanche/peermanager.h>
#include <avalanche/proofbuilder.h>
#include <avalanche/test/util.h>
#include <script/standard.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace avalanche;

BOOST_FIXTURE_TEST_SUITE(peermanager_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(select_peer_linear) {
    // No peers.
    BOOST_CHECK_EQUAL(selectPeerImpl({}, 0, 0), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl({}, 1, 3), NO_PEER);

    // One peer
    const std::vector<Slot> oneslot = {{100, 100, 23}};

    // Undershoot
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 0, 300), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 42, 300), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 99, 300), NO_PEER);

    // Nailed it
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 100, 300), 23);
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 142, 300), 23);
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 199, 300), 23);

    // Overshoot
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 200, 300), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 242, 300), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(oneslot, 299, 300), NO_PEER);

    // Two peers
    const std::vector<Slot> twoslots = {{100, 100, 69}, {300, 100, 42}};

    // Undershoot
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 0, 500), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 42, 500), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 99, 500), NO_PEER);

    // First entry
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 100, 500), 69);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 142, 500), 69);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 199, 500), 69);

    // In between
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 200, 500), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 242, 500), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 299, 500), NO_PEER);

    // Second entry
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 300, 500), 42);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 342, 500), 42);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 399, 500), 42);

    // Overshoot
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 400, 500), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 442, 500), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(twoslots, 499, 500), NO_PEER);
}

BOOST_AUTO_TEST_CASE(select_peer_dichotomic) {
    std::vector<Slot> slots;

    // 100 peers of size 1 with 1 empty element apart.
    uint64_t max = 1;
    for (int i = 0; i < 100; i++) {
        slots.emplace_back(max, 1, i);
        max += 2;
    }

    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 4, max), NO_PEER);

    // Check that we get what we expect.
    for (int i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(selectPeerImpl(slots, 2 * i, max), NO_PEER);
        BOOST_CHECK_EQUAL(selectPeerImpl(slots, 2 * i + 1, max), i);
    }

    BOOST_CHECK_EQUAL(selectPeerImpl(slots, max, max), NO_PEER);

    // Update the slots to be heavily skewed toward the last element.
    slots[99] = slots[99].withScore(101);
    max = slots[99].getStop();
    BOOST_CHECK_EQUAL(max, 300);

    for (int i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(selectPeerImpl(slots, 2 * i, max), NO_PEER);
        BOOST_CHECK_EQUAL(selectPeerImpl(slots, 2 * i + 1, max), i);
    }

    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 200, max), 99);
    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 256, max), 99);
    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 299, max), 99);
    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 300, max), NO_PEER);

    // Update the slots to be heavily skewed toward the first element.
    for (int i = 0; i < 100; i++) {
        slots[i] = slots[i].withStart(slots[i].getStart() + 100);
    }

    slots[0] = Slot(1, slots[0].getStop() - 1, slots[0].getPeerId());
    slots[99] = slots[99].withScore(1);
    max = slots[99].getStop();
    BOOST_CHECK_EQUAL(max, 300);

    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 0, max), NO_PEER);
    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 1, max), 0);
    BOOST_CHECK_EQUAL(selectPeerImpl(slots, 42, max), 0);

    for (int i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(selectPeerImpl(slots, 100 + 2 * i + 1, max), i);
        BOOST_CHECK_EQUAL(selectPeerImpl(slots, 100 + 2 * i + 2, max), NO_PEER);
    }
}

BOOST_AUTO_TEST_CASE(select_peer_random) {
    for (int c = 0; c < 1000; c++) {
        size_t size = InsecureRandBits(10) + 1;
        std::vector<Slot> slots;
        slots.reserve(size);

        uint64_t max = InsecureRandBits(3);
        auto next = [&]() {
            uint64_t r = max;
            max += InsecureRandBits(3);
            return r;
        };

        for (size_t i = 0; i < size; i++) {
            const uint64_t start = next();
            const uint32_t score = InsecureRandBits(3);
            max += score;
            slots.emplace_back(start, score, i);
        }

        for (int k = 0; k < 100; k++) {
            uint64_t s = max > 0 ? InsecureRandRange(max) : 0;
            auto i = selectPeerImpl(slots, s, max);
            // /!\ Because of the way we construct the vector, the peer id is
            // always the index. This might not be the case in practice.
            BOOST_CHECK(i == NO_PEER || slots[i].contains(s));
        }
    }
}

static std::shared_ptr<Proof> getRandomProofPtr(uint32_t score) {
    return std::make_shared<Proof>(buildRandomProof(score));
}

static void addNodeWithScore(avalanche::PeerManager &pm, NodeId node,
                             uint32_t score) {
    auto proof = getRandomProofPtr(score);
    BOOST_CHECK_NE(pm.getPeerId(proof), NO_PEER);
    Delegation dg = DelegationBuilder(*proof).build();
    BOOST_CHECK(pm.addNode(node, dg));
};

BOOST_AUTO_TEST_CASE(peer_probabilities) {
    // No peers.
    avalanche::PeerManager pm;
    BOOST_CHECK_EQUAL(pm.selectNode(), NO_NODE);

    const NodeId node0 = 42, node1 = 69, node2 = 37;

    // One peer, we always return it.
    addNodeWithScore(pm, node0, MIN_VALID_PROOF_SCORE);
    BOOST_CHECK_EQUAL(pm.selectNode(), node0);

    // Two peers, verify ratio.
    addNodeWithScore(pm, node1, 2 * MIN_VALID_PROOF_SCORE);

    std::unordered_map<PeerId, int> results = {};
    for (int i = 0; i < 10000; i++) {
        size_t n = pm.selectNode();
        BOOST_CHECK(n == node0 || n == node1);
        results[n]++;
    }

    BOOST_CHECK(abs(2 * results[0] - results[1]) < 500);

    // Three peers, verify ratio.
    addNodeWithScore(pm, node2, MIN_VALID_PROOF_SCORE);

    results.clear();
    for (int i = 0; i < 10000; i++) {
        size_t n = pm.selectNode();
        BOOST_CHECK(n == node0 || n == node1 || n == node2);
        results[n]++;
    }

    BOOST_CHECK(abs(results[0] - results[1] + results[2]) < 500);
}

BOOST_AUTO_TEST_CASE(remove_peer) {
    // No peers.
    avalanche::PeerManager pm;
    BOOST_CHECK_EQUAL(pm.selectPeer(), NO_PEER);

    // Add 4 peers.
    std::array<PeerId, 8> peerids;
    for (int i = 0; i < 4; i++) {
        auto p = getRandomProofPtr(100);
        peerids[i] = pm.getPeerId(p);
        BOOST_CHECK(
            pm.addNode(InsecureRand32(), DelegationBuilder(*p).build()));
    }

    BOOST_CHECK_EQUAL(pm.getSlotCount(), 400);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 0);

    for (int i = 0; i < 100; i++) {
        PeerId p = pm.selectPeer();
        BOOST_CHECK(p == peerids[0] || p == peerids[1] || p == peerids[2] ||
                    p == peerids[3]);
    }

    // Remove one peer, it nevers show up now.
    BOOST_CHECK(pm.removePeer(peerids[2]));
    BOOST_CHECK_EQUAL(pm.getSlotCount(), 400);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 100);

    // Make sure we compact to never get NO_PEER.
    BOOST_CHECK_EQUAL(pm.compact(), 100);
    BOOST_CHECK(pm.verify());
    BOOST_CHECK_EQUAL(pm.getSlotCount(), 300);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 0);

    for (int i = 0; i < 100; i++) {
        PeerId p = pm.selectPeer();
        BOOST_CHECK(p == peerids[0] || p == peerids[1] || p == peerids[3]);
    }

    // Add 4 more peers.
    for (int i = 0; i < 4; i++) {
        auto p = getRandomProofPtr(100);
        peerids[i + 4] = pm.getPeerId(p);
        BOOST_CHECK(
            pm.addNode(InsecureRand32(), DelegationBuilder(*p).build()));
    }

    BOOST_CHECK_EQUAL(pm.getSlotCount(), 700);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 0);

    BOOST_CHECK(pm.removePeer(peerids[0]));
    BOOST_CHECK_EQUAL(pm.getSlotCount(), 700);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 100);

    // Removing the last entry do not increase fragmentation.
    BOOST_CHECK(pm.removePeer(peerids[7]));
    BOOST_CHECK_EQUAL(pm.getSlotCount(), 600);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 100);

    // Make sure we compact to never get NO_PEER.
    BOOST_CHECK_EQUAL(pm.compact(), 100);
    BOOST_CHECK(pm.verify());
    BOOST_CHECK_EQUAL(pm.getSlotCount(), 500);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 0);

    for (int i = 0; i < 100; i++) {
        PeerId p = pm.selectPeer();
        BOOST_CHECK(p == peerids[1] || p == peerids[3] || p == peerids[4] ||
                    p == peerids[5] || p == peerids[6]);
    }

    // Removing non existent peers fails.
    BOOST_CHECK(!pm.removePeer(peerids[0]));
    BOOST_CHECK(!pm.removePeer(peerids[2]));
    BOOST_CHECK(!pm.removePeer(peerids[7]));
    BOOST_CHECK(!pm.removePeer(NO_PEER));
}

BOOST_AUTO_TEST_CASE(compact_slots) {
    avalanche::PeerManager pm;

    // Add 4 peers.
    std::array<PeerId, 4> peerids;
    for (int i = 0; i < 4; i++) {
        auto p = getRandomProofPtr(100);
        peerids[i] = pm.getPeerId(p);
        BOOST_CHECK(
            pm.addNode(InsecureRand32(), DelegationBuilder(*p).build()));
    }

    // Remove all peers.
    for (auto p : peerids) {
        pm.removePeer(p);
    }

    BOOST_CHECK_EQUAL(pm.getSlotCount(), 300);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 300);

    for (int i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(pm.selectPeer(), NO_PEER);
    }

    BOOST_CHECK_EQUAL(pm.compact(), 300);
    BOOST_CHECK(pm.verify());
    BOOST_CHECK_EQUAL(pm.getSlotCount(), 0);
    BOOST_CHECK_EQUAL(pm.getFragmentation(), 0);
}

BOOST_AUTO_TEST_CASE(node_crud) {
    avalanche::PeerManager pm;

    // Create one peer.
    auto proof = getRandomProofPtr(10000000 * MIN_VALID_PROOF_SCORE);
    BOOST_CHECK_NE(pm.getPeerId(proof), NO_PEER);
    Delegation dg = DelegationBuilder(*proof).build();
    BOOST_CHECK_EQUAL(pm.selectNode(), NO_NODE);

    // Add 4 nodes.
    for (int i = 0; i < 4; i++) {
        BOOST_CHECK(pm.addNode(i, dg));
    }

    for (int i = 0; i < 100; i++) {
        NodeId n = pm.selectNode();
        BOOST_CHECK(n >= 0 && n < 4);
        BOOST_CHECK(
            pm.updateNextRequestTime(n, std::chrono::steady_clock::now()));
    }

    // Remove a node, check that it doesn't show up.
    BOOST_CHECK(pm.removeNode(2));

    for (int i = 0; i < 100; i++) {
        NodeId n = pm.selectNode();
        BOOST_CHECK(n == 0 || n == 1 || n == 3);
        BOOST_CHECK(
            pm.updateNextRequestTime(n, std::chrono::steady_clock::now()));
    }

    // Push a node's timeout in the future, so that it doesn't show up.
    BOOST_CHECK(pm.updateNextRequestTime(1, std::chrono::steady_clock::now() +
                                                std::chrono::hours(24)));

    for (int i = 0; i < 100; i++) {
        NodeId n = pm.selectNode();
        BOOST_CHECK(n == 0 || n == 3);
        BOOST_CHECK(
            pm.updateNextRequestTime(n, std::chrono::steady_clock::now()));
    }

    // Move a node from a peer to another. This peer has a very low score such
    // as chances of being picked are 1 in 10 million.
    addNodeWithScore(pm, 3, MIN_VALID_PROOF_SCORE);

    int node3selected = 0;
    for (int i = 0; i < 100; i++) {
        NodeId n = pm.selectNode();
        if (n == 3) {
            // Selecting this node should be exceedingly unlikely.
            BOOST_CHECK(node3selected++ < 1);
        } else {
            BOOST_CHECK_EQUAL(n, 0);
        }
        BOOST_CHECK(
            pm.updateNextRequestTime(n, std::chrono::steady_clock::now()));
    }
}

BOOST_AUTO_TEST_CASE(proof_conflict) {
    CKey key;
    key.MakeNewKey(true);
    const CScript script = GetScriptForDestination(PKHash(key.GetPubKey()));

    TxId txid1(GetRandHash());
    TxId txid2(GetRandHash());
    BOOST_CHECK(txid1 != txid2);

    const Amount v = 5 * COIN;
    const int height = 1234;

    {
        LOCK(cs_main);
        CCoinsViewCache &coins = ::ChainstateActive().CoinsTip();

        for (int i = 0; i < 10; i++) {
            coins.AddCoin(COutPoint(txid1, i),
                          Coin(CTxOut(v, script), height, false), false);
            coins.AddCoin(COutPoint(txid2, i),
                          Coin(CTxOut(v, script), height, false), false);
        }
    }

    avalanche::PeerManager pm;
    const auto getPeerId = [&](const std::vector<COutPoint> &outpoints) {
        ProofBuilder pb(0, 0, CPubKey());
        for (const auto &o : outpoints) {
            pb.addUTXO(o, v, height, false, key);
        }

        return pm.getPeerId(std::make_shared<Proof>(pb.build()));
    };

    // Add one peer.
    const PeerId peer1 = getPeerId({COutPoint(txid1, 0)});
    BOOST_CHECK(peer1 != NO_PEER);

    // Same proof, same peer.
    BOOST_CHECK_EQUAL(getPeerId({COutPoint(txid1, 0)}), peer1);

    // Different txid, different proof.
    const PeerId peer2 = getPeerId({COutPoint(txid2, 0)});
    BOOST_CHECK(peer2 != NO_PEER && peer2 != peer1);

    // Different index, different proof.
    const PeerId peer3 = getPeerId({COutPoint(txid1, 1)});
    BOOST_CHECK(peer3 != NO_PEER && peer3 != peer1);

    // Empty proof, no peer.
    BOOST_CHECK_EQUAL(getPeerId({}), NO_PEER);

    // Multiple inputs.
    const PeerId peer4 = getPeerId({COutPoint(txid1, 2), COutPoint(txid2, 2)});
    BOOST_CHECK(peer4 != NO_PEER && peer4 != peer1);

    // Duplicated input.
    BOOST_CHECK_EQUAL(getPeerId({COutPoint(txid1, 3), COutPoint(txid1, 3)}),
                      NO_PEER);

    // Multiple inputs, collision on first input.
    BOOST_CHECK_EQUAL(getPeerId({COutPoint(txid1, 0), COutPoint(txid2, 4)}),
                      NO_PEER);

    // Mutliple inputs, collision on second input.
    BOOST_CHECK_EQUAL(getPeerId({COutPoint(txid1, 4), COutPoint(txid2, 0)}),
                      NO_PEER);

    // Mutliple inputs, collision on both inputs.
    BOOST_CHECK_EQUAL(getPeerId({COutPoint(txid1, 0), COutPoint(txid2, 2)}),
                      NO_PEER);
}

BOOST_AUTO_TEST_CASE(orphan_proofs) {
    avalanche::PeerManager pm;

    CKey key;
    key.MakeNewKey(true);
    const CScript script = GetScriptForDestination(PKHash(key.GetPubKey()));

    COutPoint outpoint1 = COutPoint(TxId(GetRandHash()), 0);
    COutPoint outpoint2 = COutPoint(TxId(GetRandHash()), 0);
    COutPoint outpoint3 = COutPoint(TxId(GetRandHash()), 0);

    const Amount v = 5 * COIN;
    const int height = 1234;
    const int wrongHeight = 12345;

    const auto makeProof = [&](const COutPoint &outpoint, const int h) {
        ProofBuilder pb(0, 0, CPubKey());
        pb.addUTXO(outpoint, v, h, false, key);
        return std::make_shared<Proof>(pb.build());
    };

    auto proof1 = makeProof(outpoint1, height);
    auto proof2 = makeProof(outpoint2, height);
    auto proof3 = makeProof(outpoint3, wrongHeight);

    const Coin coin = Coin(CTxOut(v, script), height, false);

    // Add outpoints 1 and 3, not 2
    {
        LOCK(cs_main);
        CCoinsViewCache &coins = ::ChainstateActive().CoinsTip();
        coins.AddCoin(outpoint1, coin, false);
        coins.AddCoin(outpoint3, coin, false);
    }

    // Add the proofs
    BOOST_CHECK(pm.getPeerId(proof1) != NO_PEER);
    BOOST_CHECK(pm.getPeerId(proof2) == NO_PEER);
    BOOST_CHECK(pm.getPeerId(proof3) == NO_PEER);

    // Good
    BOOST_CHECK(!pm.isOrphan(proof1->getId()));
    // MISSING_UTXO
    BOOST_CHECK(pm.isOrphan(proof2->getId()));
    // HEIGHT_MISMATCH
    BOOST_CHECK(pm.isOrphan(proof3->getId()));

    const auto isGoodPeer = [&pm](const std::shared_ptr<Proof> &p) {
        for (const auto &peer : pm.getPeers()) {
            if (p->getId() == peer.proof->getId()) {
                return true;
            }
        }
        return false;
    };

    BOOST_CHECK(isGoodPeer(proof1));
    BOOST_CHECK(!isGoodPeer(proof2));
    BOOST_CHECK(!isGoodPeer(proof3));

    // Add outpoint2, proof2 is no longer considered orphan
    {
        LOCK(cs_main);
        CCoinsViewCache &coins = ::ChainstateActive().CoinsTip();
        coins.AddCoin(outpoint2, coin, false);
    }

    pm.updatedBlockTip();
    BOOST_CHECK(!pm.isOrphan(proof2->getId()));
    BOOST_CHECK(isGoodPeer(proof2));

    // The status of proof1 and proof3 are unchanged
    BOOST_CHECK(!pm.isOrphan(proof1->getId()));
    BOOST_CHECK(isGoodPeer(proof1));
    BOOST_CHECK(pm.isOrphan(proof3->getId()));
    BOOST_CHECK(!isGoodPeer(proof3));

    // Spend outpoint1, proof1 becomes orphan
    {
        LOCK(cs_main);
        CCoinsViewCache &coins = ::ChainstateActive().CoinsTip();
        coins.SpendCoin(outpoint1);
    }

    pm.updatedBlockTip();
    BOOST_CHECK(pm.isOrphan(proof1->getId()));
    BOOST_CHECK(!isGoodPeer(proof1));

    // The status of proof2 and proof3 are unchanged
    BOOST_CHECK(!pm.isOrphan(proof2->getId()));
    BOOST_CHECK(isGoodPeer(proof2));
    BOOST_CHECK(pm.isOrphan(proof3->getId()));
    BOOST_CHECK(!isGoodPeer(proof3));

    // A reorg could make a previous HEIGHT_MISMATCH become valid
    {
        LOCK(cs_main);
        CCoinsViewCache &coins = ::ChainstateActive().CoinsTip();
        coins.SpendCoin(outpoint3);
        coins.AddCoin(outpoint3, Coin(CTxOut(v, script), wrongHeight, false),
                      false);
    }

    pm.updatedBlockTip();
    BOOST_CHECK(!pm.isOrphan(proof3->getId()));
    BOOST_CHECK(isGoodPeer(proof3));

    // The status of proof 1 and proof2 are unchanged
    BOOST_CHECK(pm.isOrphan(proof1->getId()));
    BOOST_CHECK(!isGoodPeer(proof1));
    BOOST_CHECK(!pm.isOrphan(proof2->getId()));
    BOOST_CHECK(isGoodPeer(proof2));
}

BOOST_AUTO_TEST_SUITE_END()
