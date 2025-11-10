# norns c/c++ testing

a shared test infrastructure for matron, crone, ws-wrapper, maiden-repl, etc.

discovery, build, and execution of c/c++ tests across all of norns components.

this document covers:
- test infrastructure (discovery, build system, directory structure)
- advanced patterns (mocking c dependencies, helper files, test seams)
- comprehensive examples and reference material

for a quick start guide to writing your first test, see [CONTRIBUTING.md](../CONTRIBUTING.md#testing-cc-code).

all c/c++ components share one test runner system. `tests/wscript` discovers `test_*.cpp` files, links matching system sources, and builds one test binary per subdirectory. no manual configuration required.

⸻

## directory structure

this is an example layout for the `tests/` directory structure to illustrate the conventions:

```
tests/
  common/
    test_main.cpp          # doctest main (shared across all tests)
  <proj>/                  # matron, crone, ws-wrapper, maiden-repl, etc.
    test_*.cpp             # project-root tests (auto-map to src/*.{c,cpp})
    common/                # optional: helper files and stubs (auto-included)
      helpers.{c,cpp}
      stubs.c
    <unit>/                # or nest by unit
      test_*.cpp           # one or more test files
      common/              # optional: helper files and stubs (auto-included)
        helpers.{c,cpp}
        stubs.c
```

**common folders**: place helper functions, mock implementations, and stubs in `common/` subdirectories. all `.c` and `.cpp` files in `common/` are automatically included when building that directory's test binary. use these for shared test utilities, c function stubs for mocking, or test-specific implementations.

## mirror mapping

test directories automatically discover system sources:

**project-root mapping**:
- `tests/<proj>/test_foo.cpp` → `<proj>/src/foo.*` or `<proj>/src/Foo.*`

the test runner tries multiple case variants for project-root tests:
- `test_window.cpp` → `window.c`, `Window.c`, `Window.cpp`, `window.cpp`
- `test_peak_meter.cpp` → `peak_meter.c`, `PeakMeter.cpp`, `peakmeter.c`, etc.

this handles both snake_case c sources and CamelCase c++ sources automatically.

**nested mapping**:
- `tests/<proj>/<unit>/` → `<proj>/src/<unit>.*` (exact file) or
- `tests/<proj>/<unit>/` → `<proj>/src/<unit>/*.{c,cpp}` (directory)

nested mappings use exact paths without case variants.

examples:
```
tests/crone/test_window.cpp              → crone/src/Window.cpp
tests/matron/test_args.cpp               → matron/src/args.c
tests/matron/clocks/test_clock_crow.cpp  → matron/src/clocks/clock_crow.c
```

## how it works

the test build is driven by `tests/wscript`, which automatically discovers tests and sources:

1. **self-test**: validates test runner logic before discovering tests
2. **find all tests**: scans for `test_*.cpp` files throughout `tests/`
3. **group by directory**: each directory becomes one test binary
4. **discover system sources**: uses mirror mapping to find matching code in `<proj>/src/`
5. **build and link**: creates standalone test binaries with doctest and system code

the test runner self-tests its own functionality on every run, verifying discovery patterns, path resolution, and source collection work correctly. skip with `--skip-self-test` if needed.

**build details:**

each directory builds one test binary combining all its `test_*.cpp` files. helper files and stubs in `common/` subdirectories are automatically included. the `NORNS_TEST` preprocessor define is set during test builds, enabling test seams in system code (see below). tests run independently with isolated failures.

## test seams

`NORNS_TEST` is a preprocessor define enabled only during test builds. it allows creating test seams in system code without affecting runtime behavior.

common use cases:
- expose private functions for testing using `#ifdef NORNS_TEST`
- inject test-only interfaces for dependency injection
- add hooks for verifying internal state

**expose private functions:**

```c
static void internal_helper() { /* ... */ }

#ifdef NORNS_TEST
void test_internal_helper() { internal_helper(); }
#endif
```

```cpp
class AudioProcessor {
private:
    void processBuffer() { /* ... */ }

#ifdef NORNS_TEST
public:
    void test_processBuffer() { processBuffer(); }
#endif
};
```

**inject test interface:**

```c
#ifdef NORNS_TEST
static struct TestCallbacks {
    void (*on_state_change)(int state);
} test_callbacks = {0};
#endif

void update_state(int new_state) {
    state = new_state;
#ifdef NORNS_TEST
    if (test_callbacks.on_state_change) {
        test_callbacks.on_state_change(new_state);
    }
#endif
}
```

```cpp
class Engine {
#ifdef NORNS_TEST
public:
    std::function<void(float)> test_onLevelChange;
#endif

    void setLevel(float level) {
        level_ = level;
#ifdef NORNS_TEST
        if (test_onLevelChange) {
            test_onLevelChange(level);
        }
#endif
    }
};
```

**verify internal state:**

```c
static int retry_count = 0;

#ifdef NORNS_TEST
int test_get_retry_count() { return retry_count; }
#endif
```

```cpp
class Connection {
private:
    int retryCount_ = 0;

#ifdef NORNS_TEST
public:
    int test_getRetryCount() const { return retryCount_; }
#endif
};
```

keep seams minimal and prefer testing through public APIs. seams help test internal state or isolate dependencies. use seams for existing code, prefer dependency injection for new code.

## mocking c dependencies

when testing c code that calls external functions, use trompeloeil to mock those dependencies. the pattern uses three components:

1. **c++ mock class** with `MAKE_MOCK*` macros
2. **global pointer** to the mock instance
3. **c stub functions** that delegate to the mock via the global pointer

### complete example

here's the complete pattern from `tests/matron/clock/test_clock.cpp`:

```cpp
#include <doctest/doctest.h>
#include <trompeloeil.hpp>

extern "C" {
#include "clock.h"
}

// 1. define c++ mock class
namespace {
struct ClockDepsMock {
    MAKE_MOCK0(internal_get_beat, double());
    MAKE_MOCK0(internal_get_tempo, double());
    MAKE_MOCK0(scheduler_reset_sync_events, void());
};

// 2. create global pointer to mock instance
static ClockDepsMock *g_mock = nullptr;
} // namespace

// 3. implement c stubs that delegate to mock
extern "C" {
double clock_internal_get_beat() {
    return g_mock ? g_mock->internal_get_beat() : 0.0;
}

double clock_internal_get_tempo() {
    return g_mock ? g_mock->internal_get_tempo() : 0.0;
}

void clock_scheduler_reset_sync_events() {
    if (g_mock) {
        g_mock->scheduler_reset_sync_events();
    }
}
}

// 4. use in tests
TEST_CASE("clock delegates to active source") {
    ClockDepsMock mock;
    g_mock = &mock;

    REQUIRE_CALL(mock, internal_get_beat()).RETURN(1.25);
    REQUIRE_CALL(mock, internal_get_tempo()).RETURN(110.0);

    clock_set_source(CLOCK_SOURCE_INTERNAL);
    CHECK(clock_get_beats() == doctest::Approx(1.25));
    CHECK(clock_get_tempo() == doctest::Approx(110.0));
}
```

### when to use

use c++ mocks when the c code under test calls functions from other modules and you need to:
- verify those calls happened
- control return values
- isolate the unit under test from its dependencies

### trompeloeil reference

```cpp
// expect exactly one call with specific return
REQUIRE_CALL(mock, function_name()).RETURN(value);

// allow any number of calls (0 or more)
ALLOW_CALL(mock, function_name()).RETURN(value);

// forbid any calls (test fails if called)
FORBID_CALL(mock, function_name());

// verify call order with sequences
trompeloeil::sequence seq;
REQUIRE_CALL(mock, first()).IN_SEQUENCE(seq);
REQUIRE_CALL(mock, second()).IN_SEQUENCE(seq);
```

reference: `tests/matron/clock/test_clock.cpp` for complete working example.

## helper files and stubs

place shared test utilities in `common/` subdirectories. all `.c` and `.cpp` files in `common/` are automatically included when building that directory's test binary.

### stub implementations

provide fake implementations of external dependencies:

```c
// tests/matron/clocks/common/abl_link_stubs.c
// provides controllable fake of ableton link API

volatile double g_stub_tempo = 120.0;
volatile double g_stub_beat = 0.0;

double abl_link_tempo(abl_link_session_state state) {
    (void)state;
    return g_stub_tempo;
}

double abl_link_beat_at_time(abl_link_session_state state, int64_t micros, double quantum) {
    (void)state;
    (void)micros;
    (void)quantum;
    return g_stub_beat;
}
```

tests manipulate the global variables to control behavior:

```cpp
extern "C" {
extern volatile double g_stub_tempo;
extern volatile double g_stub_beat;
}

TEST_CASE("link clock reads tempo from link session") {
    g_stub_tempo = 98.5;
    CHECK(clock_link_get_tempo() == doctest::Approx(98.5));
}
```

reference: `tests/matron/clocks/common/abl_link_stubs.c`
