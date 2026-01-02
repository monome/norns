from waflib.Build import BuildContext
from waflib.Tools import waf_unit_test

top = '.'
out = 'build'


class TestContext(BuildContext):
    cmd = 'test'
    fun = 'test'

def get_version_hash():
    import subprocess
    try:
        return subprocess.check_output([
            'git', 'rev-parse', '--verify', '--short', 'HEAD'
        ]).decode().strip()
    except subprocess.CalledProcessError:
        return ''

def options(opt):
    opt.load('compiler_c compiler_cxx waf_unit_test')
    opt.add_option('--desktop', action='store_true', default=False)
    opt.add_option('--release', action='store_true', default=False)
    opt.add_option('--enable-ableton-link', action='store_true', default=True)
    opt.add_option('--profile-matron', action='store_true', default=False)
    # ensure doctest prints success lines by default
    opt.parser.set_defaults(testcmd='%s --success')
    opt.add_option(
        '--test-dry-run',
        action='store_true',
        default=False,
        help='preview discovered tests without building',
    )
    opt.add_option(
        '--skip-self-test',
        action='store_true',
        default=False,
        help='skip test runner self-test validation',
    )

    opt.recurse('maiden-repl')

def configure(conf):
    conf.load('compiler_c compiler_cxx waf_unit_test')

    conf.define('VERSION_MAJOR', 0)
    conf.define('VERSION_MINOR', 0)
    conf.define('VERSION_PATCH', 0)
    conf.define('VERSION_HASH', get_version_hash())

    conf.env.PROFILE_MATRON = conf.options.profile_matron

    conf.env.append_unique('CFLAGS', ['-std=gnu11', '-Wall', '-Wextra', '-Werror'])
    conf.env.append_unique('CFLAGS', ['-g'])
    conf.env.append_unique('CXXFLAGS', ['-std=c++14'])
    conf.define('_GNU_SOURCE', 1)

    conf.check_cfg(package='alsa', args=['--cflags', '--libs'])
    conf.check_cfg(package='libudev', args=['--cflags', '--libs'])
    conf.check_cfg(package='libevdev', args=['--cflags', '--libs'])
    conf.check_cfg(package='libgpiod', args=['--cflags', '--libs'])
    conf.check_cfg(package='liblo', args=['--cflags', '--libs'])
    conf.check_cfg(package='cairo', args=['--cflags', '--libs'])
    conf.check_cfg(package='cairo-ft', args=['--cflags', '--libs'])
    conf.check_cfg(package='lua53', args=['--cflags', '--libs'])
    conf.check_cfg(package='nanomsg', args=['--cflags', '--libs'])
    conf.check_cfg(package='avahi-compat-libdns_sd', args=['--cflags', '--libs'])
    conf.check_cfg(package='sndfile', args=['--cflags', '--libs'])
    conf.check_cfg(package='jack', args=['--cflags', '--libs'])

    conf.check_cc(msg='Checking for libmonome',
        define_name='HAVE_LIBMONOME',
        mandatory=True,
        lib='monome',
        header_name='monome.h',
        uselib_store='LIBMONOME')

    if conf.options.desktop:
        conf.check_cfg(package='sdl2', args=['--cflags', '--libs'])
        conf.define('NORNS_DESKTOP', True)
    conf.env.NORNS_DESKTOP = conf.options.desktop

    if conf.options.release:
        conf.define('NORNS_RELEASE', True)
    conf.env.NORNS_RELEASE = conf.options.release

    conf.env.ENABLE_ABLETON_LINK = conf.options.enable_ableton_link
    conf.define('HAVE_ABLETON_LINK', conf.options.enable_ableton_link)

    conf.recurse('maiden-repl')

def build(bld):
    bld.recurse('matron')
    bld.recurse('maiden-repl')
    bld.recurse('ws-wrapper')
    bld.recurse('crone')
    bld.recurse('third-party')
    bld.recurse('watcher')

def test(bld):
    bld.recurse('tests')
    bld.add_post_fun(waf_unit_test.summary)
    bld.add_post_fun(waf_unit_test.set_exit_code)
