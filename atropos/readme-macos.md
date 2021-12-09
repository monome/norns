` crone` can now be built and run as a native JACK client application on mac OS. this has been tested on Mojave (10.14.2).

 note that there are a number of warnings when building with clang. these are pretty trivial (of the "unused variable" variety, from the Faust stack) and shouldn't cause any harm.

## requirements
-  JACK 2.0 server, 
http://jackaudio.org/downloads/
 
- i'm not actually sure if that installs development 

- `boost` and `liblo`, from homebrew

- `libsndfile`. with the homebrew version, i had difficulty linking ogg/vorbis/FLAC support, so ended up compiling this from source:
  - clone from here: https://github.com/erikd/libsndfile 
  - build and install using cmake with `-DENABLE_EXTERNAL_LIBS=OFF`
  
  