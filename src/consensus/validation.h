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
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_VALIDATION_H
#define BITCOIN_CONSENSUS_VALIDATION_H

#include <cassert>
#include <string>

/**
 * A "reason" why a transaction was invalid, suitable for determining whether
 * the provider of the transaction should be banned/ignored/disconnected/etc.
 */
enum class TxValidationResult {
    //! initial value. Tx has not yet been rejected
    TX_RESULT_UNSET = 0,
    //! invalid by consensus rules
    TX_CONSENSUS,
    /**
     * Invalid by a recent change to consensus rules.
     * Currently unused as there are no such consensus rule changes.
     */
    TX_RECENT_CONSENSUS_CHANGE,
    //! didn't meet our local policy rules
    TX_NOT_STANDARD,
    //! transaction was missing some of its inputs
    TX_MISSING_INPUTS,
    //! transaction spends a coinbase too early, or violates locktime/sequence
    //! locks
    TX_PREMATURE_SPEND,
    /**
     * Tx already in mempool or conflicts with a tx in the chain
     * Currently this is only used if the transaction already exists in the
     * mempool or on chain.
     */
    TX_CONFLICT,
    //! violated mempool's fee/size/descendant/etc limits
    TX_MEMPOOL_POLICY,
};

/**
 * A "reason" why a block was invalid, suitable for determining whether the
 * provider of the block should be banned/ignored/disconnected/etc.
 * These are much more granular than the rejection codes, which may be more
 * useful for some other use-cases.
 */
enum class BlockValidationResult {
    //! initial value. Block has not yet been rejected
    BLOCK_RESULT_UNSET = 0,
    //! invalid by consensus rules (excluding any below reasons)
    BLOCK_CONSENSUS,
    /**
     * Invalid by a change to consensus rules more recent than SegWit.
     * Currently unused as there are no such consensus rule changes, and any
     * download sources realistically need to support SegWit in order to provide
     * useful data, so differentiating between always-invalid and
     * invalid-by-pre-SegWit-soft-fork is uninteresting.
     */
    BLOCK_RECENT_CONSENSUS_CHANGE,
    //! this block was cached as being invalid and we didn't store the reason
    //! why
    BLOCK_CACHED_INVALID,
    //! invalid proof of work or time too old
    BLOCK_INVALID_HEADER,
    //! the block's data didn't match the data committed to by the PoW
    BLOCK_MUTATED,
    //! We don't have the previous block the checked one is built on
    BLOCK_MISSING_PREV,
    //! A block this one builds on is invalid
    BLOCK_INVALID_PREV,
    //! block timestamp was > 2 hours in the future (or our clock is bad)
    BLOCK_TIME_FUTURE,
    //! the block failed to meet one of our checkpoints
    BLOCK_CHECKPOINT,
    //! block finalization problems
    BLOCK_FINALIZATION,
};

/**
 * Template for capturing information about block/transaction validation.
 * This is instantiated by TxValidationState and BlockValidationState for
 * validation information on transactions and blocks respectively.
 */
template <typename Result> class ValidationState {
private:
    enum mode_state {
        MODE_VALID,   //!< everything ok
        MODE_INVALID, //!< network rule violation (DoS value may be set)
        MODE_ERROR,   //!< run-time error
    } m_mode{MODE_VALID};
    Result m_result{};
    std::string m_reject_reason;
    std::string m_debug_message;

public:
    bool Invalid(Result result, const std::string &reject_reason = "",
                 const std::string &debug_message = "") {
        m_result = result;
        m_reject_reason = reject_reason;
        m_debug_message = debug_message;
        if (m_mode != MODE_ERROR) {
            m_mode = MODE_INVALID;
        }
        return false;
    }

    bool Error(const std::string &reject_reason) {
        if (m_mode == MODE_VALID) {
            m_reject_reason = reject_reason;
        }
        m_mode = MODE_ERROR;
        return false;
    }
    bool IsValid() const { return m_mode == MODE_VALID; }
    bool IsInvalid() const { return m_mode == MODE_INVALID; }
    bool IsError() const { return m_mode == MODE_ERROR; }
    Result GetResult() const { return m_result; }
    std::string GetRejectReason() const { return m_reject_reason; }
    std::string GetDebugMessage() const { return m_debug_message; }
    std::string ToString() const {
        if (IsValid()) {
            return "Valid";
        }

        if (!m_debug_message.empty()) {
            return m_reject_reason + ", " + m_debug_message;
        }

        return m_reject_reason;
    }
};

class TxValidationState : public ValidationState<TxValidationResult> {};
class BlockValidationState : public ValidationState<BlockValidationResult> {};

#endif // BITCOIN_CONSENSUS_VALIDATION_H
