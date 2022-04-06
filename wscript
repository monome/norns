top = '.'
out = 'build'

def get_version_hash():
    import subprocess
    try:
        return subprocess.check_output([
            'git', 'rev-parse', '--verify', '--short', 'HEAD'
        ]).decode().strip()
    except subprocess.CalledProcessError:
        return ''

def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.add_option('--desktop', action='store_true', default=False)
    opt.add_option('--release', action='store_true', default=False,
        help='enable specific arm target architecture optimizations')
    opt.add_option('--enable-debug', action='store_true', default=False,
        help='enable debug code and emit symbols')
    opt.add_option('--enable-profile', action='store_true', default=False,
        help='emit gprof profiling data')
    opt.add_option('--enable-ableton-link', action='store_true', default=True)
    opt.add_option('--enable-lua-cjson', action='store_true', default=True)

    opt.recurse('maiden-repl')

def configure(conf):
    conf.load('compiler_c compiler_cxx')

    conf.define('VERSION_MAJOR', 0)
    conf.define('VERSION_MINOR', 0)
    conf.define('VERSION_PATCH', 0)
    conf.define('VERSION_HASH', get_version_hash())

    conf.env.append_unique('CFLAGS', ['-std=gnu11', '-Wall', '-Wextra', '-Werror'])
    conf.env.append_unique('CFLAGS', ['-g'])
    conf.env.append_unique('CXXFLAGS', ['-std=c++14'])
    conf.define('_GNU_SOURCE', 1)

    conf.check_cfg(package='alsa', args=['--cflags', '--libs'])
    conf.check_cfg(package='libudev', args=['--cflags', '--libs'])
    conf.check_cfg(package='libevdev', args=['--cflags', '--libs'])
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

    conf.check_cc(msg='Checking for nng',
        define_name='HAVE_NNG',
        mandatory=True,
        lib='nng',
        header_name='nng/nng.h',
        uselib_store='NNG')

    if conf.options.desktop:
        conf.check_cfg(package='sdl2', args=['--cflags', '--libs'])
        conf.define('NORNS_DESKTOP', True)

    conf.env.NORNS_DESKTOP = conf.options.desktop

    if conf.options.release:
        conf.define('NORNS_RELEASE', True)
    conf.env.NORNS_RELEASE = conf.options.release

    if conf.options.enable_debug:
        conf.define('NORNS_DEBUG', True)
    conf.env.NORNS_DEBUG = conf.options.enable_debug

    conf.env.NORNS_PROFILE = conf.options.enable_profile

    conf.env.ENABLE_ABLETON_LINK = conf.options.enable_ableton_link
    conf.define('HAVE_ABLETON_LINK', conf.options.enable_ableton_link)

    conf.env.ENABLE_LUA_CJSON = conf.options.enable_lua_cjson
    conf.define('HAVE_LUA_CJSON', conf.options.enable_lua_cjson)

    conf.recurse('maiden-repl')

def build(bld):
    #bld.recurse('matron')
    bld.recurse('maiden-repl')
    bld.recurse('ws-wrapper')
    #bld.recurse('crone')
    bld.recurse('third-party')
    bld.recurse('watcher')
    bld.recurse('norns')