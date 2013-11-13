// Copyright (c) 2013 Maciej Gajewski

#ifndef COROUTINES_MUTEX_HPP
#define COROUTINES_MUTEX_HPP

#include "profiling/profiling.hpp"

#include <mutex>
#include <atomic>
#include <cstdint>

namespace coroutines {


class spinlock
{
public:

    spinlock() 
    : _flag(ATOMIC_FLAG_INIT)
    { }

    spinlock(const char* name)
    : _flag(ATOMIC_FLAG_INIT)
    {
        #ifdef COROUTINES_SPINLOCKS_PROFILING
            CORO_PROF("spinlock", this, "created", name);
        #else
            (void)name;
        #endif
    }

    void lock()
    {
        #ifdef COROUTINES_SPINLOCKS_PROFILING
            // only report contested lock events
            if (try_lock())
                return;
            CORO_PROF("spinlock", this, "spinning begin");
        #endif
        while(_flag.test_and_set(std::memory_order_acquire))
            ; // spin
        #ifdef COROUTINES_SPINLOCKS_PROFILING
            CORO_PROF("spinlock", this, "spinning end");
        #endif
    }

    bool try_lock()
    {
        #ifdef COROUTINES_SPINLOCKS_PROFILING
            if (!_flag.test_and_set(std::memory_order_acquire))
            {
                CORO_PROF("spinlock", this, "locked");
                return true;
            }
            else
                return false;
        #else
            return !_flag.test_and_set(std::memory_order_acquire);
        #endif
    }

    void unlock()
    {
        #ifdef COROUTINES_SPINLOCKS_PROFILING
            CORO_PROF("spinlock", this, "unlocked");
        #endif
        _flag.clear(std::memory_order_release);
    }


private:

    std::atomic_flag _flag;
};


//typedef std::mutex mutex;
typedef spinlock mutex;

// based on folly's RWSpinLock
class rw_spinlock
{
    enum : std::int32_t { READER = 4, UPGRADED = 2, WRITER = 1 };

public:
    rw_spinlock() : _bits(0) {}

    void lock()
    {
        while (!try_lock())
            ;
    }

    // Writer is responsible for clearing up both the UPGRADED and WRITER bits.
    void unlock()
    {
        static_assert(READER > WRITER + UPGRADED, "wrong bits!");
        _bits.fetch_and(~(WRITER | UPGRADED), std::memory_order_release);
    }

    // SharedLockable Concept
    void lock_shared()
    {
        while (!try_lock_shared())
            ;
    }

    void unlock_shared()
    {
        _bits.fetch_add(-READER, std::memory_order_release);
    }

    // Downgrade the lock from writer status to reader status.
    void unlock_and_lock_shared()
    {
        _bits.fetch_add(READER, std::memory_order_acquire);
        unlock();
    }

    // UpgradeLockable Concept
    void lock_upgrade()
    {
        while (!try_lock_upgrade())
            ;
    }

    void unlock_upgrade()
    {
        _bits.fetch_add(-UPGRADED, std::memory_order_acq_rel);
    }

    // unlock upgrade and try to acquire write lock
    void unlock_upgrade_and_lock()
    {
        while (!try_unlock_upgrade_and_lock())
            ;
    }

    // unlock upgrade and read lock atomically
    void unlock_upgrade_and_lock_shared()
    {
        _bits.fetch_add(READER - UPGRADED, std::memory_order_acq_rel);
    }

    // write unlock and upgrade lock atomically
    void unlock_and_lock_upgrade()
    {
        // need to do it in two steps here -- as the UPGRADED bit might be OR-ed at
        // the same time when other threads are trying do try_lock_upgrade().
        _bits.fetch_or(UPGRADED, std::memory_order_acquire);
        _bits.fetch_add(-WRITER, std::memory_order_release);
    }


    // Attempt to acquire writer permission. Return false if we didn't get it.
    bool try_lock()
    {
        std::int32_t expect = 0;
        return _bits.compare_exchange_strong(expect, WRITER, std::memory_order_acq_rel);
    }

    // Try to get reader permission on the lock. This can fail if we
    // find out someone is a writer or upgrader.
    // Setting the UPGRADED bit would allow a writer-to-be to indicate
    // its intention to write and block any new readers while waiting
    // for existing readers to finish and release their read locks. This
    // helps avoid starving writers (promoted from upgraders).
    bool try_lock_shared()
    {
        // fetch_add is considerably (100%) faster than compare_exchange,
        // so here we are optimizing for the common (lock success) case.
        std::int32_t value = _bits.fetch_add(READER, std::memory_order_acquire);
        if (value & (WRITER|UPGRADED))
        {
            _bits.fetch_add(-READER, std::memory_order_release);
            return false;
        }
        return true;
    }

    // try to unlock upgrade and write lock atomically
    bool try_unlock_upgrade_and_lock()
    {
        std::int32_t expect = UPGRADED;
        return _bits.compare_exchange_strong(expect, WRITER, std::memory_order_acq_rel);
    }

    // try to acquire an upgradable lock.
    bool try_lock_upgrade()
    {
        std::int32_t value = _bits.fetch_or(UPGRADED, std::memory_order_acquire);

        // Note: when failed, we cannot flip the UPGRADED bit back,
        // as in this case there is either another upgrade lock or a write lock.
        // If it's a write lock, the bit will get cleared up when that lock's done
        // with unlock().
        return ((value & (UPGRADED | WRITER)) == 0);
    }

private:

    std::atomic<int32_t> _bits;
};

template<typename MutexType>
class reader_guard
{
public:
    reader_guard(MutexType& mutex)
        : _mutex(mutex)
    {
        _mutex.lock_shared();
    }

    ~reader_guard()
    {
        _mutex.unlock_shared();
    }
private:

    MutexType& _mutex;
};

}

#endif
