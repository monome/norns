// tests for norns/sidecar_shell.cpp, supervised shell execution.

#include <chrono>
#include <doctest/doctest.h>
#include <errno.h>
#include <signal.h>
#include <string>
#include <sys/resource.h>
#include <thread>
#include <unistd.h>

#include "sidecar_shell.h"

// helpers for the tests below.
namespace {

struct capture_t {
    std::string out;
    bool refuse = false;
    bool abandon = false;
    int watch_drain_fd = -1;
    int watch_calls = 0;
};

bool collect(void *ctx, const char *buf, size_t len) {
    capture_t *cap = (capture_t *)ctx;
    if (cap->refuse) {
        return false;
    }
    cap->out.append(buf, len);
    return true;
}

bool watch(void *ctx) {
    capture_t *cap = (capture_t *)ctx;
    cap->watch_calls++;
    if (cap->watch_drain_fd >= 0) {
        char b;
        read(cap->watch_drain_fd, &b, 1);
    }
    return cap->abandon;
}

sidecar_shell_opts_t opts_for(capture_t *cap) {
    sidecar_shell_opts_t opts = {};
    opts.ctx = cap;
    opts.on_output = collect;
    opts.on_watch = watch;
    opts.watch_fd = -1;
    return opts;
}

} // namespace

//---------------------------------
//--- execution

TEST_CASE("sidecar_shell: runs a command, streams its output, reports the exit status") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);

    sidecar_shell_result_t res;
    sidecar_shell_run("echo hi; exit 7", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(res.code == 7);
    CHECK(cap.out == "hi\n");
}

TEST_CASE("sidecar_shell: a command that dies on a signal is reported as signalled") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);

    sidecar_shell_result_t res;
    sidecar_shell_run("kill -KILL $$", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_SIGNALLED);
    CHECK(res.code == SIGKILL);
}

TEST_CASE("sidecar_shell: stderr rides the output stream") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);

    sidecar_shell_result_t res;
    sidecar_shell_run("echo oops >&2", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(res.code == 0);
    CHECK(cap.out == "oops\n");
}

TEST_CASE("sidecar_shell: a missing command reports 127 and the shell's complaint") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);

    sidecar_shell_result_t res;
    sidecar_shell_run("definitely_not_a_command_xyz", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(res.code == 127);
    CHECK(cap.out.find("definitely_not_a_command_xyz") != std::string::npos);
}

TEST_CASE("sidecar_shell: a command that cannot start reports the reason") {
    struct rlimit saved;
    REQUIRE(getrlimit(RLIMIT_NOFILE, &saved) == 0);
    struct rlimit tight = saved;
    tight.rlim_cur = 3;
    REQUIRE(setrlimit(RLIMIT_NOFILE, &tight) == 0);

    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);

    sidecar_shell_result_t res;
    sidecar_shell_run("echo hi", &opts, &res);

    REQUIRE(setrlimit(RLIMIT_NOFILE, &saved) == 0);

    CHECK(res.outcome == SIDECAR_SHELL_FAILED);
    CHECK(res.err != nullptr);
    CHECK(res.errnum == EMFILE);
    CHECK(cap.out.empty());
}

TEST_CASE("sidecar_shell: the command sees an empty stdin, not the caller's") {
    int stdin_pipe[2];
    REQUIRE(pipe(stdin_pipe) == 0);
    int saved_stdin = dup(STDIN_FILENO);
    REQUIRE(saved_stdin >= 0);
    REQUIRE(dup2(stdin_pipe[0], STDIN_FILENO) >= 0);

    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.silence_timeout_ms = 1000;

    sidecar_shell_result_t res;
    sidecar_shell_run("cat", &opts, &res);

    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(res.code == 0);
    CHECK(cap.out == "");
}

//---------------------------------
//--- silence timeout

TEST_CASE("sidecar_shell: a command that goes quiet past the silence budget is killed") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.silence_timeout_ms = 300;

    sidecar_shell_result_t res;
    sidecar_shell_run("sleep 10", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_QUIET);
}

TEST_CASE("sidecar_shell: output resets the silence budget, total duration is unbounded") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.silence_timeout_ms = 2000;

    sidecar_shell_result_t res;
    sidecar_shell_run("for i in 1 2 3 4 5 6; do echo t$i; sleep 0.5; done", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(res.code == 0);
    CHECK(cap.out == "t1\nt2\nt3\nt4\nt5\nt6\n");
}

//---------------------------------
//--- output limits

TEST_CASE("sidecar_shell: a command that exceeds the output cap is killed") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.max_output_bytes = 1000;

    sidecar_shell_result_t res;
    sidecar_shell_run("head -c 100000 /dev/zero", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_CAPPED);
    CHECK(cap.out.size() <= 1000);
}

TEST_CASE("sidecar_shell: the command is stopped once on_output refuses more") {
    capture_t cap;
    cap.refuse = true;
    sidecar_shell_opts_t opts = opts_for(&cap);

    sidecar_shell_result_t res;
    sidecar_shell_run("head -c 50000 /dev/zero", &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_REFUSED);
    CHECK(cap.out.empty());
}

//---------------------------------
//--- watch fd

TEST_CASE("sidecar_shell: a readable watch fd can abandon the command") {
    int wpipe[2];
    REQUIRE(pipe(wpipe) == 0);
    REQUIRE(write(wpipe[1], "x", 1) == 1);

    capture_t cap;
    cap.abandon = true;
    cap.watch_drain_fd = wpipe[0];
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.watch_fd = wpipe[0];

    auto start = std::chrono::steady_clock::now();
    sidecar_shell_result_t res;
    sidecar_shell_run("sleep 10", &opts, &res);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    close(wpipe[0]);
    close(wpipe[1]);

    CHECK(res.outcome == SIDECAR_SHELL_ABANDONED);
    CHECK(elapsed < 5000);
}

TEST_CASE("sidecar_shell: a watch wake the callback declines leaves the command alone") {
    int wpipe[2];
    REQUIRE(pipe(wpipe) == 0);
    REQUIRE(write(wpipe[1], "x", 1) == 1);

    capture_t cap;
    cap.watch_drain_fd = wpipe[0];
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.watch_fd = wpipe[0];

    sidecar_shell_result_t res;
    sidecar_shell_run("echo done", &opts, &res);

    close(wpipe[0]);
    close(wpipe[1]);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(cap.out == "done\n");
    CHECK(cap.watch_calls >= 1);
}

TEST_CASE("sidecar_shell: a dead watch fd is dropped, not spun on") {
    int dead = 900;
    close(dead);

    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.watch_fd = dead;

    struct rusage before;
    struct rusage after;
    getrusage(RUSAGE_SELF, &before);
    sidecar_shell_result_t res;
    sidecar_shell_run("sleep 1; echo ok", &opts, &res);
    getrusage(RUSAGE_SELF, &after);

    CHECK(res.outcome == SIDECAR_SHELL_EXITED);
    CHECK(cap.out == "ok\n");
    CHECK(cap.watch_calls == 0);

    long cpu_ms = (after.ru_utime.tv_sec - before.ru_utime.tv_sec) * 1000 + (after.ru_utime.tv_usec - before.ru_utime.tv_usec) / 1000 + (after.ru_stime.tv_sec - before.ru_stime.tv_sec) * 1000 + (after.ru_stime.tv_usec - before.ru_stime.tv_usec) / 1000;
    CHECK(cpu_ms < 500);
}

//---------------------------------
//--- after eof

TEST_CASE("sidecar_shell: a child that closes its output and lingers still honors the watch") {
    int wpipe[2];
    REQUIRE(pipe(wpipe) == 0);

    capture_t cap;
    cap.abandon = true;
    cap.watch_drain_fd = wpipe[0];
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.watch_fd = wpipe[0];

    std::thread poker([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        write(wpipe[1], "x", 1);
    });

    auto start = std::chrono::steady_clock::now();
    sidecar_shell_result_t res;
    sidecar_shell_run("exec >/dev/null 2>&1; sleep 10", &opts, &res);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    poker.join();
    close(wpipe[0]);
    close(wpipe[1]);

    CHECK(res.outcome == SIDECAR_SHELL_ABANDONED);
    CHECK(elapsed < 5000);
}

TEST_CASE("sidecar_shell: a child that closes its output and lingers still goes quiet") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.silence_timeout_ms = 500;

    auto start = std::chrono::steady_clock::now();
    sidecar_shell_result_t res;
    sidecar_shell_run("exec >/dev/null 2>&1; sleep 10", &opts, &res);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    CHECK(res.outcome == SIDECAR_SHELL_QUIET);
    CHECK(elapsed < 5000);
}

//---------------------------------
//--- stopping a command

TEST_CASE("sidecar_shell: a killed command gets SIGTERM first, so cleanup traps run") {
    const char *marker = "/tmp/sidecar_shell_term_marker";
    unlink(marker);

    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.silence_timeout_ms = 300;
    opts.term_grace_ms = 2000;

    sidecar_shell_result_t res;
    std::string cmd = std::string("trap 'touch ") + marker + "; exit 0' TERM; sleep 10";
    sidecar_shell_run(cmd.c_str(), &opts, &res);

    CHECK(res.outcome == SIDECAR_SHELL_QUIET);
    CHECK(access(marker, F_OK) == 0);
    unlink(marker);
}

TEST_CASE("sidecar_shell: a command that ignores SIGTERM is SIGKILLed after the grace") {
    capture_t cap;
    sidecar_shell_opts_t opts = opts_for(&cap);
    opts.silence_timeout_ms = 300;
    opts.term_grace_ms = 300;

    auto start = std::chrono::steady_clock::now();
    sidecar_shell_result_t res;
    sidecar_shell_run("trap '' TERM; for i in 1 2 3 4 5 6 7 8 9 10; do sleep 1; done", &opts, &res);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    CHECK(res.outcome == SIDECAR_SHELL_QUIET);
    CHECK(elapsed < 5000);
}
