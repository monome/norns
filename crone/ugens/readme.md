# building unit generators

## requirements

- cmake 

- supercollider sources: [ https://github.com/supercollider/supercollider.git ]

note: supercollider sources must be the same API level as the SC version you want to run the ugens in. released versions are tagged in the sc repo (e.v. `Version-3.8.0`)

## procedure

for example, building `CutFadeVoice`, given the locations `~/norns` and `~/src/supercollider`

```
cd ~/norns/crone/ugens/CutFadeVoice
mkdir build && cd build
cmake -DSC_PATH=~/src/supercollider ..
make
```

(note the ending ".." on cmake invocation)

then copy/symlink  `~norns/crone/ugens/CutFadeVoice/CutFadeVoice.sc` and `~norns/crone/ugens/CutFadeVoice/build/CutFadeVoice.so` to platform-specific SC extensions directory (e.g. `~/.local/share/Supercollider/Extensions/norns/ugens` on linux, `~/Library/Application Support/etc...` on macos)

(on macos the plugin extension is `.scx` instead of `.so`)

## (alternate procedure: CLion)

you can also use CLion - an .idea directory is included in the repo so it should "just work". CLion names the build directory something like `cmake-build-debug`