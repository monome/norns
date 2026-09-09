// tests for crone/src/Commands (concurrent command queue)

#include <atomic>
#include <doctest/doctest.h>
#include <thread>
#include <vector>

#include "Commands.h"

using namespace crone;

// ---------------------------------------------------------------------------
// single-thread round-trip

TEST_CASE("Commands enqueue/dequeue round-trip") {
    Commands cmds;
    cmds.post(Commands::Id::SET_LEVEL_DAC, 0.75f);

    // dequeue via the internal queue directly (handlePending requires a
    // client, so we mirror its logic here)
    Commands::CommandPacket p;
    REQUIRE(cmds.q.try_dequeue(p));
    CHECK(p.id == Commands::Id::SET_LEVEL_DAC);
    CHECK(p.value == doctest::Approx(0.75f));
}

TEST_CASE("Commands enqueue with indices") {
    Commands cmds;
    cmds.post(Commands::Id::SET_LEVEL_MONITOR_MIX, 2, 0.5f);

    Commands::CommandPacket p;
    REQUIRE(cmds.q.try_dequeue(p));
    CHECK(p.id == Commands::Id::SET_LEVEL_MONITOR_MIX);
    CHECK(p.idx_0 == 2);
    CHECK(p.value == doctest::Approx(0.5f));
}

TEST_CASE("Commands dequeue on empty queue returns false") {
    Commands cmds;
    Commands::CommandPacket p;
    CHECK_FALSE(cmds.q.try_dequeue(p));
}

// ---------------------------------------------------------------------------
// multi-producer, single-consumer stress test

TEST_CASE("Commands multi-producer concurrent enqueue") {
    Commands cmds;

    constexpr int NUM_PRODUCERS = 4;
    constexpr int POSTS_PER_PRODUCER = 64;

    std::atomic<int> ready{0};

    auto producer = [&](int id) {
        ready.fetch_add(1);
        // spin until all producers are ready
        while (ready.load() < NUM_PRODUCERS) {
            std::this_thread::yield();
        }
        for (int i = 0; i < POSTS_PER_PRODUCER; ++i) {
            cmds.post(Commands::Id::SET_LEVEL_DAC, id, static_cast<float>(i));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        threads.emplace_back(producer, i);
    }
    for (auto &t : threads) {
        t.join();
    }

    // drain and count
    int count = 0;
    Commands::CommandPacket p;
    while (cmds.q.try_dequeue(p)) {
        ++count;
    }

    CHECK(count == NUM_PRODUCERS * POSTS_PER_PRODUCER);
}
