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
// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKFILTER_H
#define BITCOIN_BLOCKFILTER_H

#include <primitives/block.h>
#include <primitives/blockhash.h>
#include <serialize.h>
#include <uint256.h>
#include <undo.h>
#include <util/bytevectorhash.h>

#include <cstdint>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

/**
 * This implements a Golomb-coded set as defined in BIP 158. It is a
 * compact, probabilistic data structure for testing set membership.
 */
class GCSFilter {
public:
    typedef std::vector<uint8_t> Element;
    typedef std::unordered_set<Element, ByteVectorHash> ElementSet;

    struct Params {
        uint64_t m_siphash_k0;
        uint64_t m_siphash_k1;
        uint8_t m_P;  //!< Golomb-Rice coding parameter
        uint32_t m_M; //!< Inverse false positive rate

        Params(uint64_t siphash_k0 = 0, uint64_t siphash_k1 = 0, uint8_t P = 0,
               uint32_t M = 1)
            : m_siphash_k0(siphash_k0), m_siphash_k1(siphash_k1), m_P(P),
              m_M(M) {}
    };

private:
    Params m_params;
    uint32_t m_N; //!< Number of elements in the filter
    uint64_t m_F; //!< Range of element hashes, F = N * M
    std::vector<uint8_t> m_encoded;

    /** Hash a data element to an integer in the range [0, N * M). */
    uint64_t HashToRange(const Element &element) const;

    std::vector<uint64_t> BuildHashedSet(const ElementSet &elements) const;

    /** Helper method used to implement Match and MatchAny */
    bool MatchInternal(const uint64_t *sorted_element_hashes,
                       size_t size) const;

public:
    /** Constructs an empty filter. */
    explicit GCSFilter(const Params &params = Params());

    /** Reconstructs an already-created filter from an encoding. */
    GCSFilter(const Params &params, std::vector<uint8_t> encoded_filter);

    /** Builds a new filter from the params and set of elements. */
    GCSFilter(const Params &params, const ElementSet &elements);

    uint32_t GetN() const { return m_N; }
    const Params &GetParams() const { return m_params; }
    const std::vector<uint8_t> &GetEncoded() const { return m_encoded; }

    /**
     * Checks if the element may be in the set. False positives are possible
     * with probability 1/M.
     */
    bool Match(const Element &element) const;

    /**
     * Checks if any of the given elements may be in the set. False positives
     * are possible with probability 1/M per element checked. This is more
     * efficient that checking Match on multiple elements separately.
     */
    bool MatchAny(const ElementSet &elements) const;
};

constexpr uint8_t BASIC_FILTER_P = 19;
constexpr uint32_t BASIC_FILTER_M = 784931;

enum class BlockFilterType : uint8_t {
    BASIC = 0,
    INVALID = 255,
};

/** Get the human-readable name for a filter type. Returns empty string for
 * unknown types. */
const std::string &BlockFilterTypeName(BlockFilterType filter_type);

/** Find a filter type by its human-readable name. */
bool BlockFilterTypeByName(const std::string &name,
                           BlockFilterType &filter_type);

/** Get a list of known filter types. */
const std::set<BlockFilterType> &AllBlockFilterTypes();

/** Get a comma-separated list of known filter type names. */
const std::string &ListBlockFilterTypes();

/**
 * Complete block filter struct as defined in BIP 157. Serialization matches
 * payload of "cfilter" messages.
 */
class BlockFilter {
private:
    BlockFilterType m_filter_type = BlockFilterType::INVALID;
    BlockHash m_block_hash;
    GCSFilter m_filter;

    bool BuildParams(GCSFilter::Params &params) const;

public:
    BlockFilter() = default;

    //! Reconstruct a BlockFilter from parts.
    BlockFilter(BlockFilterType filter_type, const BlockHash &block_hash,
                std::vector<uint8_t> filter);

    //! Construct a new BlockFilter of the specified type from a block.
    BlockFilter(BlockFilterType filter_type, const CBlock &block,
                const CBlockUndo &block_undo);

    BlockFilterType GetFilterType() const { return m_filter_type; }
    const BlockHash &GetBlockHash() const { return m_block_hash; }
    const GCSFilter &GetFilter() const { return m_filter; }

    const std::vector<uint8_t> &GetEncodedFilter() const {
        return m_filter.GetEncoded();
    }

    //! Compute the filter hash.
    uint256 GetHash() const;

    //! Compute the filter header given the previous one.
    uint256 ComputeHeader(const uint256 &prev_header) const;

    template <typename Stream> void Serialize(Stream &s) const {
        s << static_cast<uint8_t>(m_filter_type) << m_block_hash
          << m_filter.GetEncoded();
    }

    template <typename Stream> void Unserialize(Stream &s) {
        std::vector<uint8_t> encoded_filter;
        uint8_t filter_type;

        s >> filter_type >> m_block_hash >> encoded_filter;

        m_filter_type = static_cast<BlockFilterType>(filter_type);

        GCSFilter::Params params;
        if (!BuildParams(params)) {
            throw std::ios_base::failure("unknown filter_type");
        }
        m_filter = GCSFilter(params, std::move(encoded_filter));
    }
};

#endif // BITCOIN_BLOCKFILTER_H
