# norns
_changes. travels. is open to possibilities._

**norns is primarily intended to run on a pi-based sound computer designed by [monome](https://monome.org/norns).**

norns is many sound instruments. it connects to grids, MIDI, and other objects. norns lets you define its behavior with scripts and DSP.

## quick reference

```
git clone https://github.com/monome/norns.git
cd norns
git submodule update --init --recursive
./waf configure --release
./waf build --release
```

(NB: the `--release` flag creates builds specifically for armv8/cortex-a53 instruction set, meaning optimized for rpi3 and compatible with rpi4. It also enables aggressive compiler optimizations. Omit flag if you need debug symbols or to build for a different architecture. It does need to supplied to both configuration and build steps.)

## documentation
- [user docs](https://monome.org/docs/norns)
- [API docs](https://monome.org/docs/norns/api)
- [building and execution](readme-setup.md)
- [discussion and help](https://llllllll.co)

## related
### supporting repositories
- [maiden](https://github.com/monome/maiden/) | editing interface
- [softcut](https://github.com/monome/softcut-lib) | sample-cutting editor part of `matron`

### tools
- [norns disk image](https://github.com/monome/norns-image) | raspbian-based disk image
- [monome linux kernel](https://github.com/monome/linux/) | custom, monome-flavored linux kernel
- [DIY shield instructions](https://github.com/monome/norns-shield) | the DIY open-source hardware variant

## acknowledgments

`matron` (control system) and `crone` (audio system) were created by [@catfact](https://github.com/catfact). `maiden` (editor) was created by [@ngwese](https://github.com/ngwese). Each grew with contributions from [@artfwo](https://github.com/artfwo), [@antonhornquist](https://github.com/antonhornquist), [@simonvanderveldt](https://github.com/simonvanderveldt),[@ranch-verdin](https://github.com/ranch-verdin), [@pq](https://github.com/pq), [@markwheeler](https://github.com/markwheeler), [@csboling](https://github.com/csboling) and many others.

norns was initiated by [@tehn](https://github.com/tehn) (monome).
