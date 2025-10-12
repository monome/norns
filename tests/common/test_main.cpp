// shared test main for all C/C++ tests
// integrates doctest with trompeloeil and provides concise output defaults

#define DOCTEST_CONFIG_IMPLEMENT
#include <cstdlib>
#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <trompeloeil.hpp>

int main(int argc, char **argv) {
    // route Trompeloeil reports to doctest before tests run
    trompeloeil::set_reporter(
        [](trompeloeil::severity s,
           const char *file,
           unsigned long line,
           const std::string &msg) {
            const char *f = line ? file : "[file/line unavailable]";
            if (s == trompeloeil::severity::fatal) {
                DOCTEST_ADD_FAIL_AT(f, line, doctest::String(msg.c_str()));
            } else {
                DOCTEST_ADD_FAIL_CHECK_AT(f, line, doctest::String(msg.c_str()));
            }
        },
        [](const char *ok_msg) {
#ifdef DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
            DOCTEST_REQUIRE_UNARY(ok_msg);
#else
            DOCTEST_REQUIRE_NE(doctest::String(ok_msg), "");
#endif
        });

    doctest::Context ctx;

    // default to concise, plain output—easier to read in colored wrappers
    // opt in to verbose via `NORNS_TEST_VERBOSE`
    const char *verbose = std::getenv("NORNS_TEST_VERBOSE");
    const bool want_verbose = verbose && std::strcmp(verbose, "0") != 0 && std::strcmp(verbose, "false") != 0;
    if (!want_verbose) {
        // hide assertion successes and stop after first failure — reduce noise
        ctx.setOption("success", false);
        ctx.setOption("abort-after", 1);
    }

    // allow CLI options to override defaults
    ctx.applyCommandLine(argc, argv);
    int res = ctx.run();
    return res;
}
