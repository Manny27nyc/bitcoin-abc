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
// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCHEDULER_H
#define BITCOIN_SCHEDULER_H

#include <sync.h>

//
// NOTE:
// boost::thread should be ported to std::thread
// when we support C++11.
//
#include <condition_variable>
#include <functional>
#include <list>
#include <map>

//
// Simple class for background tasks that should be run periodically or once
// "after a while"
//
// Usage:
//
// CScheduler* s = new CScheduler();
// // Assuming a: void doSomething() { }
// s->scheduleFromNow(doSomething, std::chrono::milliseconds{11});
// s->scheduleFromNow([=] { this->func(argument); },
//                    std::chrono::milliseconds{3});
// boost::thread *t = new boost::thread(std::bind(CScheduler::serviceQueue, s));
//
// ... then at program shutdown, make sure to call stop() to clean up the
// thread(s) running serviceQueue:
// s->stop();
// t->join();
// delete t;
// delete s; // Must be done after thread is interrupted/joined.
//

class CScheduler {
public:
    CScheduler();
    ~CScheduler();

    typedef std::function<void()> Function;
    typedef std::function<bool()> Predicate;

    // Call func at/after time t
    void schedule(Function f, std::chrono::system_clock::time_point t);

    /** Call f once after the delta has passed */
    void scheduleFromNow(Function f, std::chrono::milliseconds delta) {
        schedule(std::move(f), std::chrono::system_clock::now() + delta);
    }

    /**
     * Repeat p until it return false. First run is after delta has passed once.
     *
     * The timing is not exact: Every time p is finished, it is rescheduled to
     * run again after delta. If you need more accurate scheduling, don't use
     * this method.
     */
    void scheduleEvery(Predicate p, std::chrono::milliseconds delta);

    /**
     * Mock the scheduler to fast forward in time.
     * Iterates through items on taskQueue and reschedules them
     * to be delta_seconds sooner.
     */
    void MockForward(std::chrono::seconds delta_seconds);

    // To keep things as simple as possible, there is no unschedule.

    // Services the queue 'forever'. Should be run in a thread, and interrupted
    // using boost::interrupt_thread
    void serviceQueue();

    // Tell any threads running serviceQueue to stop as soon as they're done
    // servicing whatever task they're currently servicing (drain=false) or when
    // there is no work left to be done (drain=true)
    void stop(bool drain = false);

    // Returns number of tasks waiting to be serviced,
    // and first and last task times
    size_t getQueueInfo(std::chrono::system_clock::time_point &first,
                        std::chrono::system_clock::time_point &last) const;

    // Returns true if there are threads actively running in serviceQueue()
    bool AreThreadsServicingQueue() const;

private:
    mutable Mutex newTaskMutex;
    std::condition_variable newTaskScheduled;
    std::multimap<std::chrono::system_clock::time_point, Function>
        taskQueue GUARDED_BY(newTaskMutex);
    int nThreadsServicingQueue GUARDED_BY(newTaskMutex){0};
    bool stopRequested GUARDED_BY(newTaskMutex){false};
    bool stopWhenEmpty GUARDED_BY(newTaskMutex){false};
    bool shouldStop() const EXCLUSIVE_LOCKS_REQUIRED(newTaskMutex) {
        return stopRequested || (stopWhenEmpty && taskQueue.empty());
    }
};

/**
 * Class used by CScheduler clients which may schedule multiple jobs
 * which are required to be run serially. Jobs may not be run on the
 * same thread, but no two jobs will be executed
 * at the same time and memory will be release-acquire consistent
 * (the scheduler will internally do an acquire before invoking a callback
 * as well as a release at the end). In practice this means that a callback
 * B() will be able to observe all of the effects of callback A() which executed
 * before it.
 */
class SingleThreadedSchedulerClient {
private:
    CScheduler *m_pscheduler;

    RecursiveMutex m_cs_callbacks_pending;
    std::list<std::function<void()>>
        m_callbacks_pending GUARDED_BY(m_cs_callbacks_pending);
    bool m_are_callbacks_running GUARDED_BY(m_cs_callbacks_pending) = false;

    void MaybeScheduleProcessQueue();
    void ProcessQueue();

public:
    explicit SingleThreadedSchedulerClient(CScheduler *pschedulerIn)
        : m_pscheduler(pschedulerIn) {}

    /**
     * Add a callback to be executed. Callbacks are executed serially
     * and memory is release-acquire consistent between callback executions.
     * Practially, this means that callbacks can behave as if they are executed
     * in order by a single thread.
     */
    void AddToProcessQueue(std::function<void()> func);

    // Processes all remaining queue members on the calling thread, blocking
    // until queue is empty. Must be called after the CScheduler has no
    // remaining processing threads!
    void EmptyQueue();

    size_t CallbacksPending();
};

#endif // BITCOIN_SCHEDULER_H
