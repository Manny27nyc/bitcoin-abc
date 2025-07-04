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

#ifndef BITCOIN_INTERFACES_NODE_H
#define BITCOIN_INTERFACES_NODE_H

#include <amount.h>     // For Amount
#include <net.h>        // For CConnman::NumConnections
#include <net_types.h>  // For banmap_t
#include <netaddress.h> // For Network

#include <support/allocators/secure.h> // For SecureString
#include <util/translation.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

class BanMan;
class CCoinControl;
class CFeeRate;
struct CNodeStateStats;
struct CNodeStats;
class Coin;
class Config;
class HTTPRPCRequestProcessor;
struct NodeContext;
class proxyType;
class RPCServer;
class RPCTimerInterface;
enum class SynchronizationState;
class UniValue;
enum class WalletCreationStatus;
struct bilingual_str;

namespace interfaces {
class Handler;
class Wallet;
struct BlockTip;

//! Top-level interface for a bitcoin node (bitcoind process).
class Node {
public:
    virtual ~Node() {}

    //! Init logging.
    virtual void initLogging() = 0;

    //! Init parameter interaction.
    virtual void initParameterInteraction() = 0;

    //! Get warnings.
    virtual bilingual_str getWarnings() = 0;

    //! Initialize app dependencies.
    virtual bool baseInitialize(Config &config) = 0;

    //! Start node.
    virtual bool
    appInitMain(Config &config, RPCServer &rpcServer,
                HTTPRPCRequestProcessor &httpRPCRequestProcessor) = 0;

    //! Stop node.
    virtual void appShutdown() = 0;

    //! Start shutdown.
    virtual void startShutdown() = 0;

    //! Return whether shutdown was requested.
    virtual bool shutdownRequested() = 0;

    //! Map port.
    virtual void mapPort(bool use_upnp) = 0;

    //! Get proxy.
    virtual bool getProxy(Network net, proxyType &proxy_info) = 0;

    //! Get number of connections.
    virtual size_t getNodeCount(CConnman::NumConnections flags) = 0;

    //! Get stats for connected nodes.
    using NodesStats =
        std::vector<std::tuple<CNodeStats, bool, CNodeStateStats>>;
    virtual bool getNodesStats(NodesStats &stats) = 0;

    //! Get ban map entries.
    virtual bool getBanned(banmap_t &banmap) = 0;

    //! Ban node.
    virtual bool ban(const CNetAddr &net_addr, int64_t ban_time_offset) = 0;

    //! Unban node.
    virtual bool unban(const CSubNet &ip) = 0;

    //! Disconnect node by address.
    virtual bool disconnectByAddress(const CNetAddr &net_addr) = 0;

    //! Disconnect node by id.
    virtual bool disconnectById(NodeId id) = 0;

    //! Get total bytes recv.
    virtual int64_t getTotalBytesRecv() = 0;

    //! Get total bytes sent.
    virtual int64_t getTotalBytesSent() = 0;

    //! Get mempool size.
    virtual size_t getMempoolSize() = 0;

    //! Get mempool dynamic usage.
    virtual size_t getMempoolDynamicUsage() = 0;

    //! Get header tip height and time.
    virtual bool getHeaderTip(int &height, int64_t &block_time) = 0;

    //! Get num blocks.
    virtual int getNumBlocks() = 0;

    //! Get best block hash.
    virtual BlockHash getBestBlockHash() = 0;

    //! Get last block time.
    virtual int64_t getLastBlockTime() = 0;

    //! Get verification progress.
    virtual double getVerificationProgress() = 0;

    //! Is initial block download.
    virtual bool isInitialBlockDownload() = 0;

    //! Get reindex.
    virtual bool getReindex() = 0;

    //! Get importing.
    virtual bool getImporting() = 0;

    //! Set network active.
    virtual void setNetworkActive(bool active) = 0;

    //! Get network active.
    virtual bool getNetworkActive() = 0;

    //! Estimate smart fee.
    virtual CFeeRate estimateSmartFee() = 0;

    //! Get dust relay fee.
    virtual CFeeRate getDustRelayFee() = 0;

    //! Execute rpc command.
    virtual UniValue executeRpc(Config &config, const std::string &command,
                                const UniValue &params,
                                const std::string &uri) = 0;

    //! List rpc commands.
    virtual std::vector<std::string> listRpcCommands() = 0;

    //! Set RPC timer interface if unset.
    virtual void rpcSetTimerInterfaceIfUnset(RPCTimerInterface *iface) = 0;

    //! Unset RPC timer interface.
    virtual void rpcUnsetTimerInterface(RPCTimerInterface *iface) = 0;

    //! Get unspent outputs associated with a transaction.
    virtual bool getUnspentOutput(const COutPoint &output, Coin &coin) = 0;

    //! Return default wallet directory.
    virtual std::string getWalletDir() = 0;

    //! Return available wallets in wallet directory.
    virtual std::vector<std::string> listWalletDir() = 0;

    //! Return interfaces for accessing wallets (if any).
    virtual std::vector<std::unique_ptr<Wallet>> getWallets() = 0;

    //! Attempts to load a wallet from file or directory.
    //! The loaded wallet is also notified to handlers previously registered
    //! with handleLoadWallet.
    virtual std::unique_ptr<Wallet>
    loadWallet(const CChainParams &params, const std::string &name,
               bilingual_str &error,
               std::vector<bilingual_str> &warnings) const = 0;

    //! Create a wallet from file
    virtual std::unique_ptr<Wallet>
    createWallet(const CChainParams &params, const SecureString &passphrase,
                 uint64_t wallet_creation_flags, const std::string &name,
                 bilingual_str &error, std::vector<bilingual_str> &warnings,
                 WalletCreationStatus &status) = 0;

    //! Register handler for init messages.
    using InitMessageFn = std::function<void(const std::string &message)>;
    virtual std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) = 0;

    //! Register handler for message box messages.
    using MessageBoxFn =
        std::function<bool(const bilingual_str &message,
                           const std::string &caption, unsigned int style)>;
    virtual std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) = 0;

    //! Register handler for question messages.
    using QuestionFn =
        std::function<bool(const bilingual_str &message,
                           const std::string &non_interactive_message,
                           const std::string &caption, unsigned int style)>;
    virtual std::unique_ptr<Handler> handleQuestion(QuestionFn fn) = 0;

    //! Register handler for progress messages.
    using ShowProgressFn = std::function<void(
        const std::string &title, int progress, bool resume_possible)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;

    //! Register handler for load wallet messages.
    using LoadWalletFn = std::function<void(std::unique_ptr<Wallet> wallet)>;
    virtual std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) = 0;

    //! Register handler for number of connections changed messages.
    using NotifyNumConnectionsChangedFn =
        std::function<void(int new_num_connections)>;
    virtual std::unique_ptr<Handler>
    handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) = 0;

    //! Register handler for network active messages.
    using NotifyNetworkActiveChangedFn =
        std::function<void(bool network_active)>;
    virtual std::unique_ptr<Handler>
    handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) = 0;

    //! Register handler for notify alert messages.
    using NotifyAlertChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler>
    handleNotifyAlertChanged(NotifyAlertChangedFn fn) = 0;

    //! Register handler for ban list messages.
    using BannedListChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler>
    handleBannedListChanged(BannedListChangedFn fn) = 0;

    //! Register handler for block tip messages.
    using NotifyBlockTipFn =
        std::function<void(SynchronizationState, interfaces::BlockTip tip,
                           double verification_progress)>;
    virtual std::unique_ptr<Handler>
    handleNotifyBlockTip(NotifyBlockTipFn fn) = 0;

    //! Register handler for header tip messages.
    using NotifyHeaderTipFn =
        std::function<void(SynchronizationState, interfaces::BlockTip tip,
                           double verification_progress)>;
    virtual std::unique_ptr<Handler>
    handleNotifyHeaderTip(NotifyHeaderTipFn fn) = 0;

    //! Get and set internal node context. Useful for testing, but not
    //! accessible across processes.
    virtual NodeContext *context() { return nullptr; }
    virtual void setContext(NodeContext *context) {}
};

//! Return implementation of Node interface.
std::unique_ptr<Node> MakeNode(NodeContext *context = nullptr);

//! Block tip (could be a header or not, depends on the subscribed signal).
struct BlockTip {
    int block_height;
    int64_t block_time;
    BlockHash block_hash;
};

} // namespace interfaces

#endif // BITCOIN_INTERFACES_NODE_H
