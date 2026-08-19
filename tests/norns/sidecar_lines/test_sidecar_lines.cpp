// tests for norns/sidecar_lines.cpp, the output splitter.

#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <vector>

#include "sidecar_lines.h"

// helpers for the tests below.
namespace {

using segments_t = std::vector<std::string>;

void collect(void *ctx, const char *line, size_t len) {
    ((segments_t *)ctx)->emplace_back(line, len);
}

segments_t split(const std::string &input) {
    sidecar_lines_t s;
    sidecar_lines_init(&s);
    segments_t out;
    for (char c : input) {
        sidecar_lines_push(&s, c, &out, collect);
    }
    sidecar_lines_flush(&s, &out, collect);
    return out;
}

std::string accumulated(const segments_t &segs) {
    std::string all;
    for (const auto &s : segs) {
        all += s;
    }
    return all;
}

} // namespace

TEST_CASE("sidecar_lines: a newline ends a segment and stays in it") {
    CHECK(split("a\n") == segments_t{"a\n"});
}

TEST_CASE("sidecar_lines: CRLF is one line ending, not two") {
    CHECK(split("a\r\n") == segments_t{"a\r\n"});
}

TEST_CASE("sidecar_lines: blank lines survive") {
    CHECK(split("a\n\nb\n") == segments_t{"a\n", "\n", "b\n"});
}

TEST_CASE("sidecar_lines: a lone newline is a segment") {
    CHECK(split("\n") == segments_t{"\n"});
}

TEST_CASE("sidecar_lines: a bare carriage return ends a segment and stays in it") {
    CHECK(split("a\rb\r") == segments_t{"a\r", "b\r"});
}

TEST_CASE("sidecar_lines: progress lines arrive as they are overwritten") {
    CHECK(split("In:1%\rIn:2%\rIn:3%\r") == segments_t{"In:1%\r", "In:2%\r", "In:3%\r"});
}

TEST_CASE("sidecar_lines: CRLF line endings throughout produce one segment per line") {
    CHECK(split("one\r\ntwo\r\n") == segments_t{"one\r\n", "two\r\n"});
}

TEST_CASE("sidecar_lines: an unterminated tail is emitted by the flush") {
    CHECK(split("a\nbc") == segments_t{"a\n", "bc"});
}

TEST_CASE("sidecar_lines: a trailing carriage return is emitted by the flush") {
    CHECK(split("a\rb") == segments_t{"a\r", "b"});
}

TEST_CASE("sidecar_lines: empty input emits nothing") {
    CHECK(split("").empty());
}

TEST_CASE("sidecar_lines: consecutive carriage returns each end a segment") {
    CHECK(split("\r\r\r") == segments_t{"\r", "\r", "\r"});
}

TEST_CASE("sidecar_lines: accumulated output is the command's output, byte for byte") {
    CHECK(accumulated(split("a\r\nb\r\n")) == "a\r\nb\r\n");
    CHECK(accumulated(split("a\nb\n")) == "a\nb\n");
    CHECK(accumulated(split("a\n\n\nb")) == "a\n\n\nb");
    CHECK(accumulated(split("In:1%\rIn:2%\rdone\n")) == "In:1%\rIn:2%\rdone\n");
    CHECK(accumulated(split("\r\r\r")) == "\r\r\r");
}

TEST_CASE("sidecar_lines: an overlong line spills across segments and loses nothing") {
    std::string huge(SIDECAR_LINE_BYTES * 2 + 7, 'x');
    huge += '\n';

    segments_t segs = split(huge);
    REQUIRE(segs.size() > 1);
    CHECK(accumulated(segs) == huge);
    for (const auto &s : segs) {
        CHECK(s.size() <= SIDECAR_LINE_BYTES);
    }
}

TEST_CASE("sidecar_lines: an overlong line stays separated from the next one") {
    std::string input(SIDECAR_LINE_BYTES * 2, 'x');
    input += '\n';
    input += "next\n";

    std::string all = accumulated(split(input));
    CHECK(all == input);
    CHECK(all.find("x\nnext\n") != std::string::npos);
}

TEST_CASE("sidecar_lines: the splitter recovers after an overlong line") {
    std::string input(SIDECAR_LINE_BYTES * 2, 'x');
    input += "\nshort\n";

    segments_t segs = split(input);
    CHECK(segs.back() == "short\n");
    CHECK(accumulated(segs) == input);
}

TEST_CASE("sidecar_lines: an overlong line with no newline at all loses nothing") {
    std::string huge(SIDECAR_LINE_BYTES * 3, 'x');

    CHECK(accumulated(split(huge)) == huge);
}
