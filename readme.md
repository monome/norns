# norns

- general usage documentation: [monome.org/docs/norns](https://monome.org/docs/norns).
- API docs: [monome.org/docs/norns/api](https://monome.org/docs/norns/api).
- build, install, configuration, and execution: [readme-setup.md](readme-setup.md)
- community discussion and help: [llllllll.co](https://llllllll.co)

norns is primarily intended to run on a pi-based sound computer designed by [monome](https://monome.org/norns), which has some supporting repositories:

- maiden, the editor: [github.com/monome/maiden](https://github.com/monome/maiden/)
- softcut (part of `matron`): [github.com/monome/softcut-lib](https://github.com/monome/softcut-lib)
- raspbian-based disk image: [github.com/monome/norns-image](https://github.com/monome/norns-image)
- modified linux kernel: [github.com/monome/linux/](https://github.com/monome/linux/)
- shield, the DIY open-source hardware variant: [github.com/monome/norns-shield](https://github.com/monome/norns-shield)

## quick reference

```
git clone https://github.com/monome/norns.git
cd norns
git submodule update --init --recursive
./waf configure
./waf
```

## acknowledgments

`matron` (control system) and `crone` (audio system) were created by @catfact. `maiden` (editor) was created by @ngwese. Each grew with contributions from @artfwo, @jah, @simon, @rv, @pq, @markwheeler, @csboling and many others.

norns was initiated by @tehn (monome).
