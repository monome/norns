-- lua/test/core/menu/update_test.lua
-- unit tests for the SYSTEM>UPDATE menu.

local luaunit = require('lib/test/luaunit')
local mock = require('lib/test/mock')

local update
local STAGE
local saved
local status_path
local progress_path

local function write_status(text)
  local f = assert(io.open(status_path, "w"))
  f:write(text)
  f:close()
end

local function write_progress(text)
  local f = assert(io.open(progress_path, "w"))
  f:write(text)
  f:close()
end

local function stall_until_systemd_is_asked()
  local before = norns.system_cmd.call_count()
  for _ = 1, update.install_timeout_ticks do update._check_install_status() end
  luaunit.assertEquals(norns.system_cmd.call_count(), before + 1)
  local call = norns.system_cmd.args(before + 1)
  luaunit.assertStrContains(call[1], "is-active norns-update")
  return call[2]
end

local function stall_until_systemd_replies(reply)
  stall_until_systemd_is_asked()(reply)
end

local function drawn_text()
  local out = {}
  for i = 1, screen.text.call_count() do out[#out + 1] = screen.text.args(i)[1] end
  for i = 1, screen.text_center.call_count() do out[#out + 1] = screen.text_center.args(i)[1] end
  return table.concat(out, "|")
end

local function status_exists()
  local f = io.open(status_path, "r")
  if f then
    f:close()
    return true
  end
  return false
end

TestUpdate = {}

function TestUpdate.setUp()
  saved = {
    _menu = _menu,
    screen = screen,
    norns = norns,
    _norns = _norns,
    util = util,
    releases = releases,
    _path = _path
  }

  _menu = {
    redraw = mock.spy(),
    set_page = mock.spy(),
    locked = false,
    alt = false,
    timer = {
      start = mock.spy(),
      stop = mock.spy(),
      time = 0,
      count = 0,
      event = nil,
    },
  }

  screen = {
    clear = function() end,
    level = function() end,
    move = function() end,
    text = mock.spy(),
    text_center = mock.spy(),
    text_extents = function(s) return #s * 4 end,
    update = mock.spy(),
  }

  norns = {
    disk = 1000,
    version = { update = "260000" },
    system_update = mock.spy(),
    system_cmd = mock.spy(),
    script = { clear = mock.spy() }
  }
  _norns = { execute = mock.spy(), cut_enable = mock.spy() }
  util = { os_capture = function() return "" end }
  _path = { home = "/nonexistent" }

  status_path = os.tmpname()
  os.remove(status_path)
  progress_path = os.tmpname()
  os.remove(progress_path)

  package.loaded['core/menu/update'] = nil
  update = require('core/menu/update')
  STAGE = update.constants
  update.status_path = status_path
  update.progress_path = progress_path
end

function TestUpdate.tearDown()
  if status_path then os.remove(status_path) end
  if progress_path then os.remove(progress_path) end
  package.loaded['core/menu/update'] = nil
  if saved then
    _menu, screen, norns = saved._menu, saved.screen, saved.norns
    _norns, util, releases, _path = saved._norns, saved.util, saved.releases, saved._path
  end
  update, STAGE, saved, status_path, progress_path = nil, nil, nil, nil, nil
end

function TestUpdate.test_poll_transitions_to_failure_on_failed_status()
  update.stage = STAGE.INSTALLING

  write_status("failed:binary-swap")
  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "binary-swap")
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_poll_noop_without_status()
  update.stage = STAGE.INSTALLING

  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertFalse(_menu.timer.stop.called())
end

function TestUpdate.test_poll_noop_on_empty_status_file()
  update.stage = STAGE.INSTALLING

  write_status("")
  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertFalse(_menu.timer.stop.called())
end

function TestUpdate.test_poll_ignores_non_failure_status()
  update.stage = STAGE.INSTALLING

  write_status("ok")
  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
end

function TestUpdate.test_poll_shows_installer_progress()
  update.stage = STAGE.INSTALLING

  write_progress("updating norns\n")
  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertEquals(update.message, "updating norns")
  luaunit.assertTrue(_menu.redraw.called())
end

function TestUpdate.test_poll_failure_outranks_progress()
  update.stage = STAGE.INSTALLING

  write_progress("updating norns\n")
  write_status("failed:units")
  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "units")
end

function TestUpdate.test_failed_download_lands_on_the_failure_screen()
  update.stage = STAGE.UPDATE

  update._handoff_update("wget: unable to resolve host address 'example.com'\ndownload: failed\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "download")
  luaunit.assertFalse(norns.system_update.called())
end

function TestUpdate.test_checksum_download_failure_names_the_reason()
  update.stage = STAGE.UPDATE

  update._handoff_update("saved 'bundle.tgz'\ndownload: failed checksum verification\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "checksum")
  luaunit.assertFalse(norns.system_update.called())
end

function TestUpdate.test_transport_failure_reads_as_download_failure()
  update.stage = STAGE.UPDATE

  update._handoff_update("")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "download")
  luaunit.assertFalse(norns.system_update.called())
end

function TestUpdate.test_completed_download_hands_off_detached()
  update.stage = STAGE.UPDATE

  update._handoff_update("saved 'bundle.tgz'\ndownload: ok\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertTrue(norns.system_update.called())
end

function TestUpdate.test_failed_launch_lands_on_the_failure_screen()
  local captured_on_fail
  norns.system_update = function(on_fail)
    captured_on_fail = on_fail
  end
  update.stage = STAGE.UPDATE

  update._handoff_update("download: ok\n")
  luaunit.assertEquals(update.stage, STAGE.INSTALLING)

  captured_on_fail("__detach_failed__")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "launch")
end

function TestUpdate.test_stalled_install_asks_systemd_before_giving_up()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 4

  for _ = 1, 3 do update._check_install_status() end
  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertFalse(norns.system_cmd.called())

  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertTrue(norns.system_cmd.called())
  luaunit.assertStrContains(norns.system_cmd.args(1)[1], "is-active norns-update")

  norns.system_cmd.args(1)[2]("inactive\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_running_updater_is_never_declared_timed_out()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  stall_until_systemd_replies("activating\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertNotEquals(update.failed_step, "timeout")

  update._check_install_status()
  luaunit.assertEquals(norns.system_cmd.call_count(), 1)
end

function TestUpdate.test_an_inactive_unit_is_not_mistaken_for_activating()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  stall_until_systemd_replies("inactive\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_a_failed_unit_reads_as_gone()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  stall_until_systemd_replies("failed\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_an_unknown_unit_reads_as_gone()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  stall_until_systemd_replies("unknown\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_an_empty_systemd_reply_reads_as_gone()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  stall_until_systemd_replies("")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_progress_resets_the_install_timeout()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 3

  write_progress("unpacking\n")
  update._check_install_status()
  update._check_install_status()
  update._check_install_status()
  luaunit.assertEquals(update.stage, STAGE.INSTALLING)

  write_progress("updating norns\n")
  update._check_install_status()
  update._check_install_status()
  update._check_install_status()
  luaunit.assertEquals(update.stage, STAGE.INSTALLING)

  update._check_install_status()
  luaunit.assertTrue(norns.system_cmd.called())
  norns.system_cmd.args(1)[2]("inactive\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_a_zero_byte_progress_file_is_not_progress()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  write_progress("")
  stall_until_systemd_replies("inactive\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_a_blank_progress_line_is_not_progress()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  write_progress("\n")
  stall_until_systemd_replies("inactive\n")

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "timeout")
end

function TestUpdate.test_a_marked_step_counts_as_progress_without_renaming()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  write_progress("installing packages\n0\n")
  update._check_install_status()
  update._check_install_status()

  write_progress("installing packages\n1\n")
  update._check_install_status()
  update._check_install_status()

  luaunit.assertEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertEquals(update.message, "installing packages")
  luaunit.assertFalse(norns.system_cmd.called())
end

function TestUpdate.test_dismiss_clears_status_and_returns_to_system()
  write_status("failed:libnng")
  update.stage = STAGE.INSTALL_FAILED
  _menu.locked = true

  update.key(3, 1)

  luaunit.assertFalse(status_exists())
  luaunit.assertFalse(_menu.locked)
  luaunit.assertTrue(_menu.set_page.called())
  luaunit.assertEquals(_menu.set_page.args(1)[1], "SYSTEM")
  luaunit.assertTrue(_menu.timer.stop.called())
  luaunit.assertTrue(_norns.execute.called())
  luaunit.assertEquals(_norns.execute.args(1)[1], "sudo systemctl start norns-sclang.service")
end

function TestUpdate.test_checking_screen_dismisses_to_system()
  update.stage = STAGE.CHECKING

  update.key(2, 1)

  luaunit.assertTrue(_menu.set_page.called())
  luaunit.assertEquals(_menu.set_page.args(1)[1], "SYSTEM")
end

function TestUpdate.test_settled_message_dismisses_to_system()
  update.stage = STAGE.MESSAGE

  update.key(3, 1)

  luaunit.assertTrue(_menu.set_page.called())
  luaunit.assertEquals(_menu.set_page.args(1)[1], "SYSTEM")
end

function TestUpdate.test_key_release_does_not_commit()
  write_status("failed:units")
  update.stage = STAGE.INSTALL_FAILED
  _menu.locked = true

  update.key(3, 0)

  luaunit.assertTrue(status_exists())
  luaunit.assertTrue(_menu.locked)
  luaunit.assertFalse(_menu.set_page.called())
end

function TestUpdate.test_stale_status_surfaced_on_init()
  write_status("failed:postcheck")

  update.init()

  luaunit.assertEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertEquals(update.failed_step, "postcheck")
  luaunit.assertFalse(_menu.timer.start.called())
  luaunit.assertFalse(norns.system_cmd.called())
end

function TestUpdate.test_no_internet_settles_on_the_message_screen()
  util.os_capture = function() return "ping: github.com: Temporary failure in name resolution" end

  update.init()

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "need internet.")
  luaunit.assertFalse(norns.system_cmd.called())
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_full_disk_settles_on_the_message_screen()
  norns.disk = 399

  update.init()

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "disk full. need 400M.")
  luaunit.assertFalse(norns.system_cmd.called())
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_manifest_reply_settles_the_stage()
  update.init()
  luaunit.assertEquals(update.stage, STAGE.CHECKING)
  luaunit.assertTrue(_menu.timer.start.called())

  local reply = norns.system_cmd.args(1)[2]
  reply("releases = { stable = { version = '260616' }, beta = { version = '260616' } }")

  luaunit.assertEquals(update.stage, STAGE.CONFIRM)
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_an_up_to_date_norns_is_offered_nothing()
  norns.version.update = "260616"
  update.init()

  norns.system_cmd.args(1)[2]("releases = { stable = { version = '260616' }, beta = { version = '260616' } }")

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "up to date.")
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_alt_offers_a_channel_choice_even_when_up_to_date()
  _menu.alt = true
  norns.version.update = "260616"
  update.init()

  norns.system_cmd.args(1)[2]("releases = { stable = { version = '260616' }, beta = { version = '260617' } }")

  luaunit.assertEquals(update.stage, STAGE.CONFIRM)
end

function TestUpdate.test_a_manifest_that_is_not_lua_settles_on_the_message_screen()
  update.init()

  norns.system_cmd.args(1)[2]("404: Not Found")

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "check failed. try again.")
  luaunit.assertTrue(_menu.timer.stop.called())
end

function TestUpdate.test_a_manifest_that_errors_when_run_settles_on_the_message_screen()
  update.init()

  norns.system_cmd.args(1)[2]("error('rate limited')")

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "check failed. try again.")
end

function TestUpdate.test_an_empty_manifest_reply_settles_on_the_message_screen()
  update.init()

  norns.system_cmd.args(1)[2]("")

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "check failed. try again.")
end

function TestUpdate.test_a_manifest_missing_a_channel_settles_on_the_message_screen()
  update.init()

  norns.system_cmd.args(1)[2]("releases = { stable = { version = '260616' } }")

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "check failed. try again.")
end

function TestUpdate.test_a_bad_manifest_does_not_resurrect_an_earlier_one()
  update.init()
  norns.system_cmd.args(1)[2]("releases = { stable = { version = '260616' }, beta = { version = '260616' } }")
  luaunit.assertEquals(update.stage, STAGE.CONFIRM)

  update.deinit()
  update.init()
  norns.system_cmd.args(2)[2]("local x = 1")

  luaunit.assertEquals(update.stage, STAGE.MESSAGE)
  luaunit.assertEquals(update.message, "check failed. try again.")
end

function TestUpdate.test_confirm_shuts_down_audio_and_asks_update_sh_to_download()
  releases = {
    stable = { version = "260616", url = "http://example.com/bundle.tgz", sha = "http://example.com/bundle.sha256" },
  }
  update.install = "stable"
  update.stage = STAGE.CONFIRM

  update.key(3, 1)

  luaunit.assertEquals(update.stage, STAGE.UPDATE)
  luaunit.assertTrue(_menu.locked)
  luaunit.assertTrue(norns.script.clear.called())
  luaunit.assertTrue(_norns.cut_enable.called(6))
  luaunit.assertEquals(_norns.cut_enable.args(1)[2], 0)
  luaunit.assertTrue(_norns.execute.called())
  luaunit.assertEquals(_norns.execute.args(1)[1], "sudo systemctl stop norns-sclang.service")
  luaunit.assertEquals(norns.system_cmd.args(1)[1],
    "/bin/bash /nonexistent/norns/update/update.sh download"
      .. " http://example.com/bundle.tgz http://example.com/bundle.sha256")
end

function TestUpdate.test_late_manifest_reply_leaves_the_next_page_alone()
  update.init()
  local reply = norns.system_cmd.args(1)[2]

  update.deinit()
  _menu.timer.stop.reset()
  _menu.redraw.reset()

  reply("releases = { stable = { version = '260616' }, beta = { version = '260616' } }")

  luaunit.assertFalse(_menu.timer.stop.called())
  luaunit.assertFalse(_menu.redraw.called())
  luaunit.assertEquals(update.stage, STAGE.CHECKING)
end

function TestUpdate.test_reply_from_a_previous_visit_loses_to_the_current_one()
  update.init()
  local stale_reply = norns.system_cmd.args(1)[2]

  update.deinit()
  update.init()
  luaunit.assertEquals(update.stage, STAGE.CHECKING)
  _menu.timer.stop.reset()

  stale_reply("releases = { stable = { version = '260616' }, beta = { version = '260616' } }")
  luaunit.assertEquals(update.stage, STAGE.CHECKING)
  luaunit.assertFalse(_menu.timer.stop.called())

  norns.system_cmd.args(2)[2]("releases = { stable = { version = '260616' }, beta = { version = '260616' } }")
  luaunit.assertEquals(update.stage, STAGE.CONFIRM)
end

function TestUpdate.test_late_download_reply_leaves_the_next_page_alone()
  releases = {
    stable = { version = "260616", url = "http://example.com/bundle.tgz", sha = "http://example.com/bundle.sha256" },
  }
  update.install = "stable"
  update.stage = STAGE.CONFIRM
  update.key(3, 1)

  local reply = norns.system_cmd.args(norns.system_cmd.call_count())[2]
  update.deinit()
  _menu.timer.stop.reset()
  _menu.redraw.reset()

  reply("download: ok\n")

  luaunit.assertFalse(norns.system_update.called())
  luaunit.assertNotEquals(update.stage, STAGE.INSTALLING)
  luaunit.assertFalse(_menu.timer.stop.called())
  luaunit.assertFalse(_menu.redraw.called())
end

function TestUpdate.test_failure_screen_names_the_failed_step()
  update.stage = STAGE.INSTALL_FAILED
  update.failed_step = "binary-swap"

  update.redraw()

  luaunit.assertStrContains(drawn_text(), "update failed")
  luaunit.assertStrContains(drawn_text(), "binary-swap")
  luaunit.assertStrContains(drawn_text(), "press any key")
  luaunit.assertTrue(screen.update.called())
end

function TestUpdate.test_failure_screen_still_draws_without_a_step()
  update.stage = STAGE.INSTALL_FAILED
  update.failed_step = nil

  update.redraw()

  luaunit.assertStrContains(drawn_text(), "unknown")
end

function TestUpdate.test_late_systemd_reply_leaves_the_next_page_alone()
  update.stage = STAGE.INSTALLING
  update.install_timeout_ticks = 2

  local reply = stall_until_systemd_is_asked()

  update.deinit()
  _menu.timer.stop.reset()
  _menu.redraw.reset()

  reply("inactive\n")

  luaunit.assertNotEquals(update.stage, STAGE.INSTALL_FAILED)
  luaunit.assertNotEquals(update.failed_step, "timeout")
  luaunit.assertFalse(_menu.timer.stop.called())
  luaunit.assertFalse(_menu.redraw.called())
end
