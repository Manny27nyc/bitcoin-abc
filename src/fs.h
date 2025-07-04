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
// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FS_H
#define BITCOIN_FS_H

#include <cstdio>
#include <string>
#if defined WIN32 && defined __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/** Filesystem operations and types */
namespace fs = boost::filesystem;

/** Bridge operations to C stdio */
namespace fsbridge {
FILE *fopen(const fs::path &p, const char *mode);
FILE *freopen(const fs::path &p, const char *mode, FILE *stream);

class FileLock {
public:
    FileLock() = delete;
    FileLock(const FileLock &) = delete;
    FileLock(FileLock &&) = delete;
    explicit FileLock(const fs::path &file);
    ~FileLock();
    bool TryLock();
    std::string GetReason() { return reason; }

private:
    std::string reason;
#ifndef WIN32
    int fd = -1;
#else
    // INVALID_HANDLE_VALUE
    void *hFile = (void *)-1;
#endif
};

std::string get_filesystem_error_message(const fs::filesystem_error &e);

// GNU libstdc++ specific workaround for opening UTF-8 paths on Windows.
//
// On Windows, it is only possible to reliably access multibyte file paths
// through `wchar_t` APIs, not `char` APIs. But because the C++ standard doesn't
// require ifstream/ofstream `wchar_t` constructors, and the GNU library doesn't
// provide them (in contrast to the Microsoft C++ library, see
// https://stackoverflow.com/questions/821873/how-to-open-an-stdfstream-ofstream-or-ifstream-with-a-unicode-filename/822032#822032),
// Boost is forced to fall back to `char` constructors which may not work
// properly.
//
// Work around this issue by creating stream objects with `_wfopen` in
// combination with `__gnu_cxx::stdio_filebuf`. This workaround can be removed
// with an upgrade to C++17, where streams can be constructed directly from
// `std::filesystem::path` objects.

#if defined WIN32 && defined __GLIBCXX__
class ifstream : public std::istream {
public:
    ifstream() = default;
    explicit ifstream(const fs::path &p,
                      std::ios_base::openmode mode = std::ios_base::in) {
        open(p, mode);
    }
    ~ifstream() { close(); }
    void open(const fs::path &p,
              std::ios_base::openmode mode = std::ios_base::in);
    bool is_open() { return m_filebuf.is_open(); }
    void close();

private:
    __gnu_cxx::stdio_filebuf<char> m_filebuf;
    FILE *m_file = nullptr;
};
class ofstream : public std::ostream {
public:
    ofstream() = default;
    explicit ofstream(const fs::path &p,
                      std::ios_base::openmode mode = std::ios_base::out) {
        open(p, mode);
    }
    ~ofstream() { close(); }
    void open(const fs::path &p,
              std::ios_base::openmode mode = std::ios_base::out);
    bool is_open() { return m_filebuf.is_open(); }
    void close();

private:
    __gnu_cxx::stdio_filebuf<char> m_filebuf;
    FILE *m_file = nullptr;
};
#else  // !(WIN32 && __GLIBCXX__)
typedef fs::ifstream ifstream;
typedef fs::ofstream ofstream;
#endif // WIN32 && __GLIBCXX__
};     // namespace fsbridge

#endif // BITCOIN_FS_H
