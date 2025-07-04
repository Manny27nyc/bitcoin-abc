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
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_BDB_H
#define BITCOIN_WALLET_BDB_H

#include <clientversion.h>
#include <fs.h>
#include <serialize.h>
#include <streams.h>
#include <util/system.h>
#include <wallet/db.h>

#include <db_cxx.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct bilingual_str;

static const unsigned int DEFAULT_WALLET_DBLOGSIZE = 100;
static const bool DEFAULT_WALLET_PRIVDB = true;

struct WalletDatabaseFileId {
    u_int8_t value[DB_FILE_ID_LEN];
    bool operator==(const WalletDatabaseFileId &rhs) const;
};

class BerkeleyDatabase;

class BerkeleyEnvironment {
private:
    bool fDbEnvInit;
    bool fMockDb;
    // Don't change into fs::path, as that can result in
    // shutdown problems/crashes caused by a static initialized internal
    // pointer.
    std::string strPath;

public:
    std::unique_ptr<DbEnv> dbenv;
    std::map<std::string, int> mapFileUseCount;
    std::map<std::string, std::reference_wrapper<BerkeleyDatabase>> m_databases;
    std::unordered_map<std::string, WalletDatabaseFileId> m_fileids;
    std::condition_variable_any m_db_in_use;

    BerkeleyEnvironment(const fs::path &env_directory);
    BerkeleyEnvironment();
    ~BerkeleyEnvironment();
    void Reset();

    void MakeMock();
    bool IsMock() const { return fMockDb; }
    bool IsInitialized() const { return fDbEnvInit; }
    bool IsDatabaseLoaded(const std::string &db_filename) const {
        return m_databases.find(db_filename) != m_databases.end();
    }
    fs::path Directory() const { return strPath; }

    bool Verify(const std::string &strFile);

    bool Open(bilingual_str &error);
    void Close();
    void Flush(bool fShutdown);
    void CheckpointLSN(const std::string &strFile);

    void CloseDb(const std::string &strFile);
    void ReloadDbEnv();

    DbTxn *TxnBegin(int flags = DB_TXN_WRITE_NOSYNC) {
        DbTxn *ptxn = nullptr;
        int ret = dbenv->txn_begin(nullptr, &ptxn, flags);
        if (!ptxn || ret != 0) {
            return nullptr;
        }
        return ptxn;
    }
};

/** Get BerkeleyEnvironment and database filename given a wallet path. */
std::shared_ptr<BerkeleyEnvironment>
GetWalletEnv(const fs::path &wallet_path, std::string &database_filename);

/** Return whether a BDB wallet database is currently loaded. */
bool IsBDBWalletLoaded(const fs::path &wallet_path);

/**
 * An instance of this class represents one database.
 * For BerkeleyDB this is just a (env, strFile) tuple.
 */
class BerkeleyDatabase {
    friend class BerkeleyBatch;

public:
    /** Create dummy DB handle */
    BerkeleyDatabase()
        : nUpdateCounter(0), nLastSeen(0), nLastFlushed(0),
          nLastWalletUpdate(0), env(nullptr) {}

    /** Create DB handle to real database */
    BerkeleyDatabase(std::shared_ptr<BerkeleyEnvironment> envIn,
                     std::string filename)
        : nUpdateCounter(0), nLastSeen(0), nLastFlushed(0),
          nLastWalletUpdate(0), env(std::move(envIn)),
          strFile(std::move(filename)) {
        auto inserted =
            this->env->m_databases.emplace(strFile, std::ref(*this));
        assert(inserted.second);
    }

    ~BerkeleyDatabase() {
        if (env) {
            size_t erased = env->m_databases.erase(strFile);
            assert(erased == 1);
        }
    }

    /**
     * Rewrite the entire database on disk, with the exception of key pszSkip if
     * non-zero
     */
    bool Rewrite(const char *pszSkip = nullptr);

    /**
     * Back up the entire database to a file.
     */
    bool Backup(const std::string &strDest) const;

    /**
     * Make sure all changes are flushed to disk.
     */
    void Flush(bool shutdown);

    /*
     * flush the wallet passively (TRY_LOCK)
     * ideal to be called periodically
     */
    bool PeriodicFlush();

    void IncrementUpdateCounter();

    void ReloadDbEnv();

    std::atomic<unsigned int> nUpdateCounter;
    unsigned int nLastSeen;
    unsigned int nLastFlushed;
    int64_t nLastWalletUpdate;

    /** Verifies the environment and database file */
    bool Verify(bilingual_str &error);

    /**
     * Pointer to shared database environment.
     *
     * Normally there is only one BerkeleyDatabase object per
     * BerkeleyEnvivonment, but in the special, backwards compatible case where
     * multiple wallet BDB data files are loaded from the same directory, this
     * will point to a shared instance that gets freed when the last data file
     * is closed.
     */
    std::shared_ptr<BerkeleyEnvironment> env;

    /**
     * Database pointer. This is initialized lazily and reset during flushes,
     * so it can be null.
     */
    std::unique_ptr<Db> m_db;

private:
    std::string strFile;

    /**
     * Return whether this database handle is a dummy for testing.
     * Only to be used at a low level, application should ideally not care
     * about this.
     */
    bool IsDummy() const { return env == nullptr; }
};

/** RAII class that provides access to a Berkeley database */
class BerkeleyBatch {
    /** RAII class that automatically cleanses its data on destruction */
    class SafeDbt final {
        Dbt m_dbt;

    public:
        // construct Dbt with internally-managed data
        SafeDbt();
        // construct Dbt with provided data
        SafeDbt(void *data, size_t size);
        ~SafeDbt();

        // delegate to Dbt
        const void *get_data() const;
        u_int32_t get_size() const;

        // conversion operator to access the underlying Dbt
        operator Dbt *();
    };

private:
    bool ReadKey(CDataStream &key, CDataStream &value);
    bool WriteKey(CDataStream &key, CDataStream &value, bool overwrite = true);
    bool EraseKey(CDataStream &key);
    bool HasKey(CDataStream &key);

protected:
    Db *pdb;
    std::string strFile;
    DbTxn *activeTxn;
    bool fReadOnly;
    bool fFlushOnClose;
    BerkeleyEnvironment *env;

public:
    explicit BerkeleyBatch(BerkeleyDatabase &database,
                           const char *pszMode = "r+",
                           bool fFlushOnCloseIn = true);
    ~BerkeleyBatch() { Close(); }

    BerkeleyBatch(const BerkeleyBatch &) = delete;
    BerkeleyBatch &operator=(const BerkeleyBatch &) = delete;

    void Flush();
    void Close();

    template <typename K, typename T> bool Read(const K &key, T &value) {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        bool success = false;
        bool ret = ReadKey(ssKey, ssValue);
        if (ret) {
            // Unserialize value
            try {
                ssValue >> value;
                success = true;
            } catch (const std::exception &) {
                // In this case success remains 'false'
            }
        }
        return ret && success;
    }

    template <typename K, typename T>
    bool Write(const K &key, const T &value, bool fOverwrite = true) {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;

        // Write
        return WriteKey(ssKey, ssValue, fOverwrite);
    }

    template <typename K> bool Erase(const K &key) {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        // Erase
        return EraseKey(ssKey);
    }

    template <typename K> bool Exists(const K &key) {
        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;

        // Exists
        return HasKey(ssKey);
    }

    Dbc *GetCursor();
    int ReadAtCursor(Dbc *pcursor, CDataStream &ssKey, CDataStream &ssValue);
    bool TxnBegin();
    bool TxnCommit();
    bool TxnAbort();
};

#endif // BITCOIN_WALLET_BDB_H
