/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013-2014, 2020 Andrey Semashev
 */
/*!
 * \file   lock_pool.cpp
 *
 * This file contains implementation of the lock pool used to emulate atomic ops.
 */

#include <cstddef>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/capabilities.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/intptr.hpp>
#include <boost/atomic/detail/operations_lockfree.hpp>
#include <boost/atomic/detail/lock_pool.hpp>
#include <boost/atomic/detail/pause.hpp>

#if BOOST_ATOMIC_FLAG_LOCK_FREE != 2
#if defined(BOOST_HAS_PTHREADS)
#include <pthread.h>
#define BOOST_ATOMIC_USE_PTHREAD
#else
#error Boost.Atomic: Unsupported target platform, POSIX threads are required when native atomic operations are not available
#endif
#endif // BOOST_ATOMIC_FLAG_LOCK_FREE != 2

#if defined(BOOST_MSVC)
#pragma warning(push)
// 'struct_name' : structure was padded due to __declspec(align())
#pragma warning(disable: 4324)
#endif

namespace boost {
namespace atomics {
namespace detail {
namespace lock_pool {

namespace {

// Cache line size, in bytes
// NOTE: This constant is made as a macro because some compilers (gcc 4.4 for one) don't allow enums or namespace scope constants in alignment attributes
#if defined(__s390__) || defined(__s390x__)
#define BOOST_ATOMIC_CACHE_LINE_SIZE 256
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
#define BOOST_ATOMIC_CACHE_LINE_SIZE 128
#else
#define BOOST_ATOMIC_CACHE_LINE_SIZE 64
#endif

#if defined(BOOST_ATOMIC_USE_PTHREAD)
typedef pthread_mutex_t lock_type;
#else
typedef atomics::detail::operations< 1u, false > lock_operations;
typedef lock_operations::storage_type lock_type;
#endif

enum
{
    padding_size = (sizeof(lock_type) <= BOOST_ATOMIC_CACHE_LINE_SIZE ?
        (BOOST_ATOMIC_CACHE_LINE_SIZE - sizeof(lock_type)) :
        (BOOST_ATOMIC_CACHE_LINE_SIZE - sizeof(lock_type) % BOOST_ATOMIC_CACHE_LINE_SIZE))
};

template< unsigned int PaddingSize >
struct BOOST_ALIGNMENT(BOOST_ATOMIC_CACHE_LINE_SIZE) padded_lock
{
    lock_type lock;
    // The additional padding is needed to avoid false sharing between locks
    char padding[PaddingSize];
};

template< >
struct BOOST_ALIGNMENT(BOOST_ATOMIC_CACHE_LINE_SIZE) padded_lock< 0u >
{
    lock_type lock;
};

typedef padded_lock< padding_size > padded_lock_t;

//! Lock pool size. Must be a power of two.
BOOST_CONSTEXPR_OR_CONST std::size_t lock_pool_size = 64u;

static padded_lock_t g_lock_pool[lock_pool_size]
#if defined(BOOST_ATOMIC_USE_PTHREAD)
=
{
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER },
    { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }, { PTHREAD_MUTEX_INITIALIZER }
}
#endif
;

//! Returns index of the lock pool entry for the given pointer value
BOOST_FORCEINLINE std::size_t get_lock_index(atomics::detail::uintptr_t ptr)
{
    // Since many malloc/new implementations return pointers with higher alignment
    // than indicated by storage_alignment, it makes sense to mix higher bits
    // into the lower ones. On 64-bit platforms, malloc typically aligns to 16 bytes,
    // on 32-bit - to 8 bytes.
    BOOST_CONSTEXPR_OR_CONST unsigned int mix_shift = sizeof(void*) >= 8u ? 4u : 3u;
    ptr ^= ptr >> mix_shift;
    return ptr & (lock_pool_size - 1u);
}

} // namespace


#if !defined(BOOST_ATOMIC_USE_PTHREAD)

// NOTE: This function must NOT be inline. Otherwise MSVC 9 will sometimes generate broken code for modulus operation which result in crashes.
BOOST_ATOMIC_DECL void* lock(atomics::detail::uintptr_t ptr) BOOST_NOEXCEPT
{
    lock_type& lock = g_lock_pool[get_lock_index(ptr)].lock;
    while (lock_operations::test_and_set(lock, memory_order_acquire))
    {
        do
        {
            atomics::detail::pause();
        }
        while (!!lock_operations::load(lock, memory_order_relaxed));
    }

    return &lock;
}

BOOST_ATOMIC_DECL void unlock(void* p) BOOST_NOEXCEPT
{
    lock_operations::clear(*static_cast< lock_type* >(p), memory_order_release);
}

#else // !defined(BOOST_ATOMIC_USE_PTHREAD)

BOOST_ATOMIC_DECL void* lock(atomics::detail::uintptr_t ptr) BOOST_NOEXCEPT
{
    lock_type& lock = g_lock_pool[get_lock_index(ptr)].lock;
    BOOST_VERIFY(pthread_mutex_lock(&lock) == 0);
    return &lock;
}

BOOST_ATOMIC_DECL void unlock(void* p) BOOST_NOEXCEPT
{
    BOOST_VERIFY(pthread_mutex_unlock(static_cast< lock_type* >(p)) == 0);
}

#endif // !defined(BOOST_ATOMIC_USE_PTHREAD)

BOOST_ATOMIC_DECL void thread_fence() BOOST_NOEXCEPT
{
#if BOOST_ATOMIC_THREAD_FENCE > 0
    atomics::detail::thread_fence(memory_order_seq_cst);
#else
    // Emulate full fence by locking/unlocking a mutex
    lock_pool::unlock(lock_pool::lock(0u));
#endif
}

BOOST_ATOMIC_DECL void signal_fence() BOOST_NOEXCEPT
{
    // This function is intentionally non-inline, even if empty. This forces the compiler to treat its call as a compiler barrier.
#if BOOST_ATOMIC_SIGNAL_FENCE > 0
    atomics::detail::signal_fence(memory_order_seq_cst);
#endif
}

} // namespace lock_pool
} // namespace detail
} // namespace atomics
} // namespace boost

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif
