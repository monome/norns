TODO: the tests should be made more easily runnable. currently one must manually
run each container test script individually by:

- `cd norns/lua`
- `LUA_PATH='lib/?.lua' lua5.3 lib/container/<name_of_container>_test.lua -v`

NB: norns/matron embed lua5.3 while the base os install only includes the lua5.1
interpreter. to test using lua5.3 as above run `sudo apt-get install lua5.3`
