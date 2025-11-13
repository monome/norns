contributing
==================================

welcome
-------

we gladly accept contributions via github [pull requests](https://help.github.com/articles/about-pull-requests/).

things you will need
--------------------

 * a [git](https://git-scm.com/) client (used for source version control).
 * a [github](https://github.com/) account (to contribute changes).
 * an [ssh](https://en.wikipedia.org/wiki/Secure_Shell) client (used to authenticate with github).

getting the code and configuring your environment
-------------------------------------------------

 * ensure all the dependencies described in the previous section are installed.
 * fork `https://github.com/monome/norns` into your own github account (more on forking
   [here](https://help.github.com/articles/fork-a-repo/)).
 * if you haven't configured your machine with an ssh key that's known to github then follow
   these [directions](https://help.github.com/articles/generating-ssh-keys/).
 * navigate to a local directory to hold your sources.
 * `git clone https://github.com/<your_name_here>/norns.git`
 * `cd norns`
 * `git remote add upstream https://github.com/monome/norns.git` (so that you
   fetch from the main repository, not your clone, when running `git fetch`
   et al.)

contributing code
-----------------

to start working on a patch:

 * `git fetch upstream`
 * `git checkout upstream/main -b name_of_your_branch`
 * hack away
 * `git commit -a -m "<your brief but informative commit message>"`
 * `git push origin name_of_your_branch`

to send us a pull request (pr):

 * go to [`https://github.com/monome/norns`](https://github.com/monome/norns)
   and click the "Compare & pull request" button.
 * be sure and include a description of the proposed change and reference any
   related issues or folks; note that if the change is significant, consider
   opening a corresponding github [issue](https://help.github.com/articles/about-issues/) 
   to discuss. (for some basic advice on writing a pr, see the github's 
   [notes on writing a perfect pr](https://blog.github.com/2015-01-21-how-to-write-the-perfect-pull-request/).)

once everyone is happy, a maintainer will merge your pr for you.

api docs for main
-------------------

to view the api docs for the `main` branch,
visit https://monome.org/docs/norns/api.

testing c/c++ code
------------------

### quick start

run all tests:
```bash
./test.sh
```

run specific test:
```bash
# example: run matron `clock.h` tests
./test.sh --targets=test_matron_clock
```

see [tests/README.md](tests/README.md) for infrastructure details, advanced patterns, and comprehensive examples.

### writing your first test

tests mirror source structure. create `test_*.cpp` files, and they get picked up by the test runner automatically:

```
tests/matron/test_args.cpp               → matron/src/args.c
tests/crone/test_window.cpp              → crone/src/Window.cpp
```

**test C code:**

```cpp
#include <doctest/doctest.h>

extern "C" {
#include "args.h"
}

TEST_CASE("args_parse handles local port") {
    char *argv[] = {"matron", "-l", "8888", nullptr};
    args_parse(3, argv);
    CHECK(std::string(args_local_port()) == "8888");
}
```

**test C++ code:**

```cpp
#include <doctest/doctest.h>
#include "Window.h"

TEST_CASE("window starts at zero") {
    CHECK(Window::raisedCosShort[0] == doctest::Approx(0.0f));
}
```

**assertions:**
- `CHECK(expr)` — non-fatal
- `REQUIRE(expr)` — fatal, stops test
- `CHECK(value == doctest::Approx(expected))` — floating point
