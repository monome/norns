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
    opt.load('compiler_c compiler_cxx boost')
    opt.add_option('--desktop', action='store_true', default=False)
    opt.add_option('--supercollider-prefix', action='store', default='/usr')

def configure(conf):
    conf.load('compiler_c compiler_cxx boost')

    conf.define('VERSION_MAJOR', 0)
    conf.define('VERSION_MINOR', 0)
    conf.define('VERSION_PATCH', 0)
    conf.define('VERSION_HASH', get_version_hash())

    conf.env.append_unique('CFLAGS', ['-std=gnu11', '-Wall', '-Wextra', '-Werror'])
    conf.env.append_unique('CFLAGS', ['-g'])
    conf.env.append_unique('CXXFLAGS', ['-std=c++11'])
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

    conf.check_cc(msg='Checking for libmonome',
        define_name='HAVE_LIBMONOME',
        mandatory=True,
        lib='monome',
        header_name='monome.h',
        uselib_store='LIBMONOME')

    conf.env.SC_PREFIX = conf.options.supercollider_prefix

    conf.check_cxx(msg='Checking for supercollider',
        define_name='HAVE_SUPERCOLLIDER',
        mandatory=True,
        includes=[
            '{}/include/SuperCollider/plugin_interface'.format(conf.env.SC_PREFIX),
            '{}/include/SuperCollider/common'.format(conf.env.SC_PREFIX),
	    '{}/sc/external_libraries/nova-simd'.format(top)
        ],
        header_name='SC_PlugIn.h',
        uselib_store='SUPERCOLLIDER')

    conf.check_boost()
    if conf.options.desktop:
        conf.check_cfg(package='sdl2', args=['--cflags', '--libs'])
        conf.define('NORNS_DESKTOP', True)

def build(bld):
    bld.recurse('matron')
    bld.recurse('maiden-repl')
    bld.recurse('ws-wrapper')
    bld.recurse('sc')
    bld.recurse('crone')
