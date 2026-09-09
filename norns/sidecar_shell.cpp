#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "sidecar_shell.h"

#define SHELL_POLL_MS 10000 // 10 s
#define SHELL_AWAIT_EXIT_POLL_MS 100

static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static void fail(sidecar_shell_result_t *result, const char *err, int errnum) {
    result->outcome = SIDECAR_SHELL_FAILED;
    result->code = 0;
    result->err = err;
    result->errnum = errnum;
}

static pid_t shell_spawn(const char *cmd, int *out_fd) {
    int fds[2];
    if (pipe(fds) != 0) {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        int err = errno;
        close(fds[0]);
        close(fds[1]);
        errno = err;
        return -1;
    }

    if (pid == 0) {
        setpgid(0, 0);
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[0]);
        close(fds[1]);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }

    setpgid(pid, pid);
    close(fds[1]);
    fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    *out_fd = fds[0];
    return pid;
}

static bool went_quiet(const sidecar_shell_opts_t *opts, uint64_t last_output, int *wait) {
    if (opts->silence_timeout_ms == 0) {
        return false;
    }
    uint64_t silent = now_ms() - last_output;
    if (silent >= opts->silence_timeout_ms) {
        return true;
    }
    uint64_t remaining = opts->silence_timeout_ms - silent;
    if (remaining < (uint64_t)*wait) {
        *wait = (int)remaining;
    }
    return false;
}

static bool watch_fired(const sidecar_shell_opts_t *opts, short revents, int *watch_fd) {
    if (revents == 0) {
        return false;
    }
    if (revents & POLLIN) {
        return opts->on_watch(opts->ctx);
    }
    *watch_fd = -1;
    return false;
}

static bool shell_exited(pid_t pid, int *status) {
    pid_t waited = waitpid(pid, status, WNOHANG);
    return waited == pid || (waited < 0 && errno != EINTR);
}

static void shell_await_exit(pid_t pid, int *status, uint64_t kill_at, bool killed) {
    for (;;) {
        if (shell_exited(pid, status)) {
            return;
        }
        int wait = SHELL_AWAIT_EXIT_POLL_MS;
        if (!killed) {
            uint64_t now = now_ms();
            if (now >= kill_at) {
                kill(-pid, SIGKILL);
                killed = true;
            } else if (kill_at - now < (uint64_t)wait) {
                wait = (int)(kill_at - now);
            }
        }
        poll(NULL, 0, wait);
    }
}

static void shell_stop(pid_t pid, int fd, uint32_t term_grace_ms) {
    kill(-pid, SIGTERM);
    uint64_t kill_at = now_ms() + term_grace_ms;
    bool killed = false;
    if (fd >= 0) {
        for (;;) {
            uint64_t now = now_ms();
            if (!killed && now >= kill_at) {
                kill(-pid, SIGKILL);
                killed = true;
            }
            int wait = killed ? SHELL_POLL_MS : (int)(kill_at - now);
            struct pollfd pfd = {fd, POLLIN, 0};
            int ready = poll(&pfd, 1, wait);
            if (ready < 0 && errno == EINTR) {
                continue;
            }
            if (ready < 0) {
                break;
            }
            if (ready == 0) {
                continue;
            }
            char scrap[4096];
            ssize_t n = read(fd, scrap, sizeof(scrap));
            if (n < 0 && errno == EINTR) {
                continue;
            }
            if (n <= 0) {
                break;
            }
        }
        close(fd);
    }
    int status;
    shell_await_exit(pid, &status, kill_at, killed);
}

void sidecar_shell_run(const char *cmd, const sidecar_shell_opts_t *opts, sidecar_shell_result_t *result) {
    result->code = 0;
    result->err = NULL;
    result->errnum = 0;

    int fd = -1;
    pid_t pid = shell_spawn(cmd, &fd);
    if (pid < 0) {
        fail(result, "could not start command", errno);
        return;
    }

    uint64_t last_output = now_ms();
    size_t total = 0;
    int watch_fd = opts->watch_fd;
    bool aborted = false;
    sidecar_shell_outcome_t why = SIDECAR_SHELL_FAILED;
    const char *err = NULL;
    int err_errno = 0;

    for (;;) {
        int wait = SHELL_POLL_MS;
        if (went_quiet(opts, last_output, &wait)) {
            aborted = true;
            why = SIDECAR_SHELL_QUIET;
            break;
        }

        struct pollfd pfds[2] = {{fd, POLLIN, 0}, {watch_fd, POLLIN, 0}};
        const nfds_t nfds = watch_fd >= 0 ? 2 : 1;
        int ready = poll(pfds, nfds, wait);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            aborted = true;
            err = "poll() failed";
            err_errno = errno;
            break;
        }
        if (ready == 0) {
            continue;
        }

        if (nfds > 1 && watch_fired(opts, pfds[1].revents, &watch_fd)) {
            aborted = true;
            why = SIDECAR_SHELL_ABANDONED;
            break;
        }

        if (pfds[0].revents == 0) {
            continue;
        }

        char buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            aborted = true;
            err = "read failed";
            err_errno = errno;
            break;
        }
        if (n == 0) {
            break;
        }

        total += (size_t)n;
        if (opts->max_output_bytes != 0 && total > opts->max_output_bytes) {
            aborted = true;
            why = SIDECAR_SHELL_CAPPED;
            break;
        }

        last_output = now_ms();
        if (!opts->on_output(opts->ctx, buf, (size_t)n)) {
            aborted = true;
            why = SIDECAR_SHELL_REFUSED;
            break;
        }
    }

    int status = 0;

    if (!aborted) {
        close(fd);
        fd = -1;
        for (;;) {
            if (shell_exited(pid, &status)) {
                break;
            }
            int wait = SHELL_AWAIT_EXIT_POLL_MS;
            if (went_quiet(opts, last_output, &wait)) {
                aborted = true;
                why = SIDECAR_SHELL_QUIET;
                break;
            }
            struct pollfd pfd = {watch_fd, POLLIN, 0};
            int ready = poll(watch_fd >= 0 ? &pfd : NULL, watch_fd >= 0 ? 1 : 0, wait);
            if (ready > 0 && watch_fired(opts, pfd.revents, &watch_fd)) {
                aborted = true;
                why = SIDECAR_SHELL_ABANDONED;
                break;
            }
        }
    }

    if (aborted) {
        shell_stop(pid, fd, opts->term_grace_ms);
        if (err != NULL) {
            fail(result, err, err_errno);
        } else {
            result->outcome = why;
        }
        return;
    }

    if (WIFEXITED(status)) {
        result->outcome = SIDECAR_SHELL_EXITED;
        result->code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result->outcome = SIDECAR_SHELL_SIGNALLED;
        result->code = WTERMSIG(status);
    } else {
        fail(result, "command ended in an unknown way", 0);
    }
}
