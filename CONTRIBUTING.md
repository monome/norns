contributing
==================================

[![Build Status](https://travis-ci.org/monome/norns.svg?branch=master)](https://travis-ci.org/monome/norns)

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
   fetch from the master repository, not your clone, when running `git fetch`
   et al.)

contributing code
-----------------

to start working on a patch:

 * `git fetch upstream`
 * `git checkout upstream/master -b name_of_your_branch`
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

api docs for master
-------------------

to view the api docs for the `master` branch,
visit https://monome.github.io/norns/doc/.

those docs should be updated after a successful travis build
of the norns `master` branch.


build infrastructure
--------------------

continuous integration of `norns` is on

- [travis](https://travis-ci.org/) ([details](.travis.yml))
