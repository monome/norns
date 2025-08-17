--- test/test_runner.lua
-- finds and runs tests using LuaUnit.
--
-- run directly to discover and run all tests:
--   lua test_runner.lua
--
-- or import as module for custom test workflows:
--   local runner = require('test.test_runner')
--   runner.run()  -- run all tests
--   runner.discover()  -- find test files
--

local luaunit = require('lib/test/luaunit')
local mock = require('lib/test/mock')

-- test files to exclude from automatic discovery
-- these are run separately during the test pipeline
local excluded_tests = {
  "lua/test/lib/test/mock_test.lua"
}

--- check if file is in exclusion list.
-- @tparam string file_path path to test file
-- @treturn boolean true if file should be excluded
local function is_excluded(file_path)
  for _, excluded in ipairs(excluded_tests) do
    if file_path == excluded then
      return true
    end
  end
  return false
end

--- discover test files in directory.
-- search for files matching *_test.lua pattern and validate readability.
-- @tparam string dir directory path to search
-- @treturn table sorted array of readable test file paths
local function discover_tests(dir)
  local tests = {}
  local cmd = 'find "' .. dir .. '" -name "*_test.lua" 2>/dev/null'
  local handle = io.popen(cmd)

  if handle then
    for line in handle:lines() do
      line = line:gsub("\\", "/")
      -- verify file exists and is readable
      local f = io.open(line, "r")
      if f then
        f:close()
        table.insert(tests, line)
      end
    end
    handle:close()
  end

  table.sort(tests)
  return tests
end

--- self-tests for the test runner.
-- validate core functionality using mocked `io` operations.
TestRunner = {}

function TestRunner.setUp()
  -- mock io.popen to return predictable test file output
  local mock_output = "lua/test/util_test.lua\nlua/test/core_test.lua\nlua/test/lib/observable_test.lua\n"
  local mock_handle = {
    lines = function()
      local lines = {}
      for line in mock_output:gmatch("[^\n]+") do
        table.insert(lines, line)
      end
      local i = 0
      return function()
        i = i + 1
        return lines[i]
      end
    end,
    close = function() end
  }

  TestRunner.restore_io_popen = mock.stub(io, 'popen', { mock_handle })

  -- mock io.open to simulate all test files as readable
  local mock_file = { close = function() end }
  TestRunner.restore_io_open = mock.stub(io, 'open', { mock_file })
end

function TestRunner.tearDown()
  if TestRunner.restore_io_popen then
    TestRunner.restore_io_popen()
    TestRunner.restore_io_popen = nil
  end
  if TestRunner.restore_io_open then
    TestRunner.restore_io_open()
    TestRunner.restore_io_open = nil
  end
end

--- test `discover_tests` returns table.
-- @within TestRunner
function TestRunner.test_discover_returns_table()
  local result = discover_tests("lua/test")
  luaunit.assertEquals(type(result), 'table')
end

--- test `discover_tests` returns sorted files.
-- @within TestRunner
function TestRunner.test_discover_returns_sorted_list()
  local result = discover_tests("lua/test")

  luaunit.assertTrue(#result >= 3, 'should have mocked test files')
  -- files should be sorted alphabetically
  luaunit.assertEquals(result[1], 'lua/test/core_test.lua')
  luaunit.assertEquals(result[2], 'lua/test/lib/observable_test.lua')
  luaunit.assertEquals(result[3], 'lua/test/util_test.lua')
end

--- test `discover_tests` finds only `*_test.lua` files.
-- @within TestRunner
function TestRunner.test_discover_only_finds_test_files()
  local result = discover_tests("lua/test")

  for _, file in ipairs(result) do
    luaunit.assertTrue(file:match('_test%.lua$') ~= nil, 'should only find *_test.lua files: ' .. file)
  end
end

--- test `discover_tests` handles missing directories.
-- @within TestRunner
function TestRunner.test_discover_nonexistent_directory()
  -- temporarily override mocks for this test
  if TestRunner.restore_io_popen then TestRunner.restore_io_popen() end
  if TestRunner.restore_io_open then TestRunner.restore_io_open() end

  -- mock io.popen to return nil (command failed)
  TestRunner.restore_io_popen = mock.stub(io, 'popen', { nil })
  TestRunner.restore_io_open = mock.stub(io, 'open', { nil })

  local result = discover_tests('/nonexistent/path')
  luaunit.assertEquals(type(result), 'table')
  luaunit.assertEquals(#result, 0, 'should return empty table when io.popen fails')
end

--- run self-tests for the test runner.
-- execute TestRunner test suite with full output.
-- @treturn number 0 on success, 1 on failure
local function run_self_tests()
  print("\27[3mvalidating test runner - running self-tests:\27[0m")
  print("---")

  local runner = luaunit.LuaUnit:new()
  local result = runner:runSuite("TestRunner")

  if result == 0 then
    print("\27[32mtest runner self-tests passed\27[0m")
  else
    print("\27[3;31mtest runner self-tests failed:\27[0m")
    print("---")
    print("the test runner cannot validate its own functionality.")
    print("this suggests an issue with test discovery or core logic within the test runner.")
    print("please check the test runner implementation for potential issues.")
    return 1
  end

  return 0
end

--- run mock library tests.
-- execute mock test suite to validate mock functionality.
-- @treturn number 0 on success, 1 on failure
local function run_mock_tests()
  print("\27[3mvalidating test mock library:\27[0m")
  print("---")

  -- load mock tests
  local success, err = pcall(dofile, "lua/test/lib/test/mock_test.lua")
  if not success then
    print("\27[1;31m✗ failed to load mock tests\27[0m")
    print("\27[31merror:\27[0m " .. err)
    return 1
  end

  local runner = luaunit.LuaUnit:new()
  local result = runner:runSuite("TestMock")

  if result == 0 then
    print("\27[32mmock library tests passed\27[0m")
  else
    print("\27[31mmock library tests failed:\27[0m")
    print("the mock library at lib/test/mock.lua is not working correctly.")
    print("this may affect other tests that depend on mocking.")
    print("fix mock issues before running full test suite.")
    return 1
  end

  return 0
end

--- load and run all discovered tests.
-- complete test pipeline: self-tests, discovery, loading, running.
-- @treturn number exit code (0 success, 1 failure)
local function run_all_tests()
  -- run self-tests first
  local self_test_result = run_self_tests()
  if self_test_result ~= 0 then
    return self_test_result
  end
  print()

  -- run mock tests
  local mock_test_result = run_mock_tests()
  if mock_test_result ~= 0 then
    return mock_test_result
  end
  print()

  -- discover test files
  io.write("\27[3mdiscovering tests…\27[0m")
  io.flush()

  local all_test_files = discover_tests("lua/test")

  -- filter out excluded test files
  local test_files = {}
  for _, file in ipairs(all_test_files) do
    if not is_excluded(file) then
      table.insert(test_files, file)
    end
  end

  io.write("\r\27[3mdiscovering tests… \27[32m✓\27[0m\n")
  io.flush()
  print("---")

  if #test_files == 0 then
    print("\27[33mno test files found\27[0m")
    print("add *_test.lua files to lua/test/ subdirectories")
    print()
    return 0
  end

  print("found " .. #test_files .. " test file(s):")
  for _, file in ipairs(test_files) do
    print("  \27[36m" .. file .. "\27[0m")
  end
  print()

  -- load and run each test file
  for _, file in ipairs(test_files) do
    local success, err = pcall(dofile, file)
    if not success then
      print()
      print("\27[1;31m✗ failed to load test file\27[0m")
      print("\27[36m" .. file .. "\27[0m")
      print()
      print("\27[31merror:\27[0m " .. err)
      print()
      return 1
    end
  end

  print("\27[3mrunning tests:\27[0m")
  print("---")

  local exit_code = luaunit.LuaUnit.run()

  print()
  if exit_code == 0 then
    print("\27[1;32m✓ all tests passed\27[0m")
    print("norns lua test suite is healthy")
  else
    print("\27[1;31m✗ some tests failed\27[0m")
    print("check test output above for specific failures")
    print("fix issues and run tests again")
  end

  return exit_code
end

-- run automatically when executed as a script (not when imported as module)
if arg and arg[0] and arg[0]:match("test_runner%.lua$") and not package.loaded["test.test_runner"] then
  os.exit(run_all_tests())
end

return {
  run = run_all_tests,
  discover = discover_tests,
}
