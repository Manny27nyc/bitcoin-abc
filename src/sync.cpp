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
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync.h>

#include <logging.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/threadnames.h>

#include <tinyformat.h>

#include <map>
#include <set>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef DEBUG_LOCKCONTENTION
void PrintLockContention(const char *pszName, const char *pszFile, int nLine) {
    LogPrintf("LOCKCONTENTION: %s\n", pszName);
    LogPrintf("Locker: %s:%d\n", pszFile, nLine);
}
#endif /* DEBUG_LOCKCONTENTION */

#ifdef DEBUG_LOCKORDER
//
// Early deadlock detection.
// Problem being solved:
//    Thread 1 locks A, then B, then C
//    Thread 2 locks D, then C, then A
//     --> may result in deadlock between the two threads, depending on when
//     they run.
// Solution implemented here:
// Keep track of pairs of locks: (A before B), (A before C), etc.
// Complain if any thread tries to lock in a different order.
//

struct CLockLocation {
    CLockLocation(const char *pszName, const char *pszFile, int nLine,
                  bool fTryIn, const std::string &thread_name)
        : fTry(fTryIn), mutexName(pszName), sourceFile(pszFile),
          m_thread_name(thread_name), sourceLine(nLine) {}

    std::string ToString() const {
        return strprintf("%s %s:%s%s (in thread %s)", mutexName, sourceFile,
                         sourceLine, (fTry ? " (TRY)" : ""), m_thread_name);
    }

    std::string Name() const { return mutexName; }

private:
    bool fTry;
    std::string mutexName;
    std::string sourceFile;
    const std::string &m_thread_name;
    int sourceLine;
};

using LockStackItem = std::pair<void *, CLockLocation>;
using LockStack = std::vector<LockStackItem>;
using LockStacks = std::unordered_map<std::thread::id, LockStack>;

using LockPair = std::pair<void *, void *>;
using LockOrders = std::map<LockPair, LockStack>;
using InvLockOrders = std::set<LockPair>;

struct LockData {
    LockStacks m_lock_stacks;
    LockOrders lockorders;
    InvLockOrders invlockorders;
    std::mutex dd_mutex;
};

LockData &GetLockData() {
    // This approach guarantees that the object is not destroyed until after its
    // last use. The operating system automatically reclaims all the memory in a
    // program's heap when that program exits.
    // Since the ~LockData() destructor is never called, the LockData class and
    // all its subclasses must have implicitly-defined destructors.
    static LockData &lock_data = *new LockData();
    return lock_data;
}

static void potential_deadlock_detected(const LockPair &mismatch,
                                        const LockStack &s1,
                                        const LockStack &s2) {
    LogPrintf("POTENTIAL DEADLOCK DETECTED\n");
    LogPrintf("Previous lock order was:\n");
    for (const LockStackItem &i : s2) {
        if (i.first == mismatch.first) {
            LogPrintfToBeContinued(" (1)");
        }
        if (i.first == mismatch.second) {
            LogPrintfToBeContinued(" (2)");
        }
        LogPrintf(" %s\n", i.second.ToString());
    }
    LogPrintf("Current lock order is:\n");
    for (const LockStackItem &i : s1) {
        if (i.first == mismatch.first) {
            LogPrintfToBeContinued(" (1)");
        }
        if (i.first == mismatch.second) {
            LogPrintfToBeContinued(" (2)");
        }
        LogPrintf(" %s\n", i.second.ToString());
    }
    if (g_debug_lockorder_abort) {
        tfm::format(
            std::cerr,
            "Assertion failed: detected inconsistent lock order at %s:%i, "
            "details in debug log.\n",
            __FILE__, __LINE__);
        abort();
    }
    throw std::logic_error("potential deadlock detected");
}

static void push_lock(void *c, const CLockLocation &locklocation) {
    LockData &lockdata = GetLockData();
    std::lock_guard<std::mutex> lock(lockdata.dd_mutex);

    LockStack &lock_stack = lockdata.m_lock_stacks[std::this_thread::get_id()];
    lock_stack.emplace_back(c, locklocation);
    for (const LockStackItem &i : lock_stack) {
        if (i.first == c) {
            break;
        }

        const LockPair p1 = std::make_pair(i.first, c);
        if (lockdata.lockorders.count(p1)) {
            continue;
        }
        lockdata.lockorders.emplace(p1, lock_stack);

        const LockPair p2 = std::make_pair(c, i.first);
        lockdata.invlockorders.insert(p2);
        if (lockdata.lockorders.count(p2)) {
            potential_deadlock_detected(p1, lockdata.lockorders[p2],
                                        lockdata.lockorders[p1]);
        }
    }
}

static void pop_lock() {
    LockData &lockdata = GetLockData();
    std::lock_guard<std::mutex> lock(lockdata.dd_mutex);

    LockStack &lock_stack = lockdata.m_lock_stacks[std::this_thread::get_id()];
    lock_stack.pop_back();
    if (lock_stack.empty()) {
        lockdata.m_lock_stacks.erase(std::this_thread::get_id());
    }
}

void EnterCritical(const char *pszName, const char *pszFile, int nLine,
                   void *cs, bool fTry) {
    push_lock(cs, CLockLocation(pszName, pszFile, nLine, fTry,
                                util::ThreadGetInternalName()));
}

void CheckLastCritical(void *cs, std::string &lockname, const char *guardname,
                       const char *file, int line) {
    {
        LockData &lockdata = GetLockData();
        std::lock_guard<std::mutex> lock(lockdata.dd_mutex);

        const LockStack &lock_stack =
            lockdata.m_lock_stacks[std::this_thread::get_id()];
        if (!lock_stack.empty()) {
            const auto &lastlock = lock_stack.back();
            if (lastlock.first == cs) {
                lockname = lastlock.second.Name();
                return;
            }
        }
    }
    throw std::system_error(
        EPERM, std::generic_category(),
        strprintf("%s:%s %s was not most recent critical section locked", file,
                  line, guardname));
}

void LeaveCritical() {
    pop_lock();
}

std::string LocksHeld() {
    LockData &lockdata = GetLockData();
    std::lock_guard<std::mutex> lock(lockdata.dd_mutex);

    const LockStack &lock_stack =
        lockdata.m_lock_stacks[std::this_thread::get_id()];
    std::string result;
    for (const LockStackItem &i : lock_stack) {
        result += i.second.ToString() + std::string("\n");
    }
    return result;
}

static bool LockHeld(void *mutex) {
    LockData &lockdata = GetLockData();
    std::lock_guard<std::mutex> lock(lockdata.dd_mutex);

    const LockStack &lock_stack =
        lockdata.m_lock_stacks[std::this_thread::get_id()];
    for (const LockStackItem &i : lock_stack) {
        if (i.first == mutex) {
            return true;
        }
    }

    return false;
}

template <typename MutexType>
void AssertLockHeldInternal(const char *pszName, const char *pszFile, int nLine,
                            MutexType *cs) {
    if (LockHeld(cs)) {
        return;
    }
    tfm::format(std::cerr,
                "Assertion failed: lock %s not held in %s:%i; locks held:\n%s",
                pszName, pszFile, nLine, LocksHeld());
    abort();
}
template void AssertLockHeldInternal(const char *, const char *, int, Mutex *);
template void AssertLockHeldInternal(const char *, const char *, int,
                                     RecursiveMutex *);

void AssertLockNotHeldInternal(const char *pszName, const char *pszFile,
                               int nLine, void *cs) {
    if (!LockHeld(cs)) {
        return;
    }
    tfm::format(std::cerr,
                "Assertion failed: lock %s held in %s:%i; locks held:\n%s",
                pszName, pszFile, nLine, LocksHeld());
    abort();
}

void DeleteLock(void *cs) {
    LockData &lockdata = GetLockData();
    std::lock_guard<std::mutex> lock(lockdata.dd_mutex);
    const LockPair item = std::make_pair(cs, nullptr);
    LockOrders::iterator it = lockdata.lockorders.lower_bound(item);
    while (it != lockdata.lockorders.end() && it->first.first == cs) {
        const LockPair invitem =
            std::make_pair(it->first.second, it->first.first);
        lockdata.invlockorders.erase(invitem);
        lockdata.lockorders.erase(it++);
    }
    InvLockOrders::iterator invit = lockdata.invlockorders.lower_bound(item);
    while (invit != lockdata.invlockorders.end() && invit->first == cs) {
        const LockPair invinvitem = std::make_pair(invit->second, invit->first);
        lockdata.lockorders.erase(invinvitem);
        lockdata.invlockorders.erase(invit++);
    }
}

bool g_debug_lockorder_abort = true;

#endif /* DEBUG_LOCKORDER */
