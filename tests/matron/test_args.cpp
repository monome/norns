// tests for matron/src/args.c behavior
// covers argument parsing, defaults, overrides, and error handling

#include <doctest/doctest.h>

#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern "C" {
#include "args.h"
}

// -----------------------------------------------------------------------------
// test helpers

static void reset_args_to_defaults() {
    char program[] = "matron";
    char opt_l[] = "-l";
    char val_l[] = "8888";
    char opt_e[] = "-e";
    char val_e[] = "57120";
    char opt_o[] = "-o";
    char val_o[] = "10111";
    char opt_c[] = "-c";
    char val_c[] = "9999";
    char opt_f[] = "-f";
    char val_f[] = "/dev/fb0";

    char *argv[] = {program, opt_l, val_l, opt_e, val_e, opt_o, val_o, opt_c, val_c, opt_f, val_f, nullptr};
    int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);

    optind = 1;
    args_parse(argc, argv);
}

// -----------------------------------------------------------------------------
// default values

TEST_CASE("args_parse returns default ports and framebuffer") {
    reset_args_to_defaults();

    CHECK(std::string(args_local_port()) == "8888");
    CHECK(std::string(args_ext_port()) == "57120");
    CHECK(std::string(args_crone_port()) == "9999");
    CHECK(std::string(args_remote_port()) == "10111");
    CHECK(std::string(args_framebuffer_path()) == "/dev/fb0");
}

// -----------------------------------------------------------------------------
// override behavior

TEST_CASE("args_parse overrides all ports and framebuffer") {
    reset_args_to_defaults();

    char program[] = "matron";
    char opt_l[] = "-l";
    char val_l[] = "12000";
    char opt_e[] = "-e";
    char val_e[] = "42000";
    char opt_o[] = "-o";
    char val_o[] = "33333";
    char opt_c[] = "-c";
    char val_c[] = "44444";
    char opt_f[] = "-f";
    char val_f[] = "/dev/fb1";
    char *argv[] = {program, opt_l, val_l, opt_e, val_e, opt_o, val_o, opt_c, val_c, opt_f, val_f, nullptr};
    int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);
    optind = 1;

    args_parse(argc, argv);

    CHECK(std::string(args_local_port()) == "12000");
    CHECK(std::string(args_ext_port()) == "42000");
    CHECK(std::string(args_remote_port()) == "33333");
    CHECK(std::string(args_crone_port()) == "44444");
    CHECK(std::string(args_framebuffer_path()) == "/dev/fb1");
}

TEST_CASE("args_parse preserves defaults when partially overridden") {
    reset_args_to_defaults();

    char program[] = "matron";
    char opt_l[] = "-l";
    char val_l[] = "12345";
    char *argv[] = {program, opt_l, val_l, nullptr};
    int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);
    optind = 1;

    args_parse(argc, argv);

    CHECK(std::string(args_local_port()) == "12345");
    CHECK(std::string(args_ext_port()) == "57120");
    CHECK(std::string(args_remote_port()) == "10111");
    CHECK(std::string(args_crone_port()) == "9999");
    CHECK(std::string(args_framebuffer_path()) == "/dev/fb0");
}

// -----------------------------------------------------------------------------
// boundary conditions

TEST_CASE("args_parse truncates values longer than 63 characters") {
    reset_args_to_defaults();

    char program[] = "matron";
    char opt_l[] = "-l";
    std::string long_port(80, '7');
    std::vector<char> long_buf(long_port.begin(), long_port.end());
    long_buf.push_back('\0');
    char *argv[] = {program, opt_l, long_buf.data(), nullptr};
    int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);
    optind = 1;

    int rc = args_parse(argc, argv);

    CHECK(rc == 0);
    std::string parsed = args_local_port();
    CHECK(parsed.size() <= 63);
    CHECK(parsed == long_port.substr(0, parsed.size()));
}

TEST_CASE("args_parse accepts values at 63 character boundary") {
    reset_args_to_defaults();

    char program[] = "matron";
    char opt_l[] = "-l";
    std::string boundary_port(63, '9');
    std::vector<char> boundary_buf(boundary_port.begin(), boundary_port.end());
    boundary_buf.push_back('\0');
    char *argv[] = {program, opt_l, boundary_buf.data(), nullptr};
    int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);
    optind = 1;

    int rc = args_parse(argc, argv);

    CHECK(rc == 0);
    std::string parsed = args_local_port();
    CHECK(parsed.size() == 63);
    CHECK(parsed == boundary_port);
}

// -----------------------------------------------------------------------------
// error handling

TEST_CASE("args_parse exits with status 1 on help flag") {
    pid_t pid = fork();
    REQUIRE(pid >= 0);

    if (pid == 0) {
        char program[] = "matron";
        char opt_h[] = "-h";
        char *argv[] = {program, opt_h, nullptr};
        int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);
        optind = 1;
        args_parse(argc, argv);
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 1);
}

TEST_CASE("args_parse exits with status 1 on unknown option") {
    pid_t pid = fork();
    REQUIRE(pid >= 0);

    if (pid == 0) {
        char program[] = "matron";
        char opt_z[] = "-z";
        char *argv[] = {program, opt_z, nullptr};
        int argc = static_cast<int>((sizeof(argv) / sizeof(argv[0])) - 1);
        optind = 1;
        args_parse(argc, argv);
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 1);
}
