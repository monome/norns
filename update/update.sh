#!/usr/bin/env bash
# @name update.sh
# @brief download, verify, install, and reboot into an updated norns release.
# @description
#   usage:
#     update.sh download <tgz-url> <sha-url>   fetch a release and its checksum
#                                              into /home/we/update and verify them
#     update.sh   (currently installed copy)   verify the downloaded release,
#                                              unpack, run its update.sh, reboot
#     update.sh   (inside an unpacked release) install that release only
#
#   the SYSTEM>UPDATE menu drives download, then runs the installed copy
#   detached. manual runs take the same paths.
#
#   output goes to UPDATE_LOG. the active norns binary shows PROGRESS_FILE and
#   STATUS_FILE (failed:<step> when a critical step aborts) on screen.
#   no failure path reboots.

#---------------------------------
#--- paths and constants

readonly ROOT="${NORNS_UPDATE_ROOT:-}"
readonly SUDO="${NORNS_UPDATE_SUDO-sudo}" # bare - default, so an empty override means run without sudo

readonly STATUS_FILE="$ROOT/tmp/norns-update-status"
readonly PROGRESS_FILE="$ROOT/tmp/norns-update-progress"
readonly UPDATE_LOG="$ROOT/tmp/norns-update.log"
readonly STAGED_COPY="$ROOT/tmp/norns-update-staged.sh" # the updater runs from this copy, the install replaces the original
readonly WE_DIR="$ROOT/home/we"
readonly UPDATE_DIR="$WE_DIR/update"
readonly NORNS_DIR="$WE_DIR/norns"
readonly MAIDEN_DIR="$WE_DIR/maiden"
readonly NORNS_BINARY=build/norns/norns # relative to a tree root
readonly UPDATE_UNIT=norns-update
readonly MIN_DISK_VERSION=220306 # older disk images need a reflash, not an update
readonly MIN_SWAP_KB=200000      # headroom to stage norns.new (~149MB tree @260102 + growth)
readonly LAUNCH_TIMEOUT=5
readonly HEARTBEAT_SECS="${NORNS_UPDATE_HEARTBEAT_SECS:-15}" # override for the dry-run suite, positive seconds only
readonly DUST_AUDIO_COMMON_TGZ=dust-audio-common.tgz
readonly DUST_AUDIO_COMMON_URL=https://github.com/monome/norns/releases/download/v2.7.1/$DUST_AUDIO_COMMON_TGZ

#---------------------------------
#--- status and progress

# @description write failed:$1 to STATUS_FILE, log the reason, exit the script.
# @arg $1 string step name used in the failed:<step> handoff
# @arg $2 string human-readable reason
# @stderr a FATAL line
# @exitcode 1 always, this function never returns
fail() {
	echo "failed:$1" > "$STATUS_FILE"
	echo "update: FATAL ($1): $2" >&2
	exit 1
}

# @description write a step name and a mark to PROGRESS_FILE. the watching
#   norns shows the name and treats a changed mark as proof of progress.
# @arg $1 string progress text
# @arg $2 int mark, counted up by the heartbeat
# @exitcode 0 always, write errors are swallowed
write_progress() {
	printf '%s\n%s\n' "$1" "$2" 2>/dev/null > "$PROGRESS_FILE" || true
}

# @description stop the heartbeat started for the previous step, if any.
# @noargs
# @set HEARTBEAT_PID string emptied once the ticker is gone
# @exitcode 0 always
heartbeat_stop() {
	[ -n "${HEARTBEAT_PID:-}" ] || return 0
	kill "$HEARTBEAT_PID" 2>/dev/null
	wait "$HEARTBEAT_PID" 2>/dev/null
	HEARTBEAT_PID=""
}

# @description re-mark PROGRESS_FILE every HEARTBEAT_SECS until the step ends.
#   apt, dpkg, and the various builds can run for many quiet moments, and
#   without this the watching norns cannot tell them from a stalled install.
# @arg $1 string progress text to keep writing
# @set HEARTBEAT_PID int pid of the ticker
# @exitcode 0 always
heartbeat_start() {
	(
		beat=0
		while sleep "$HEARTBEAT_SECS"; do
			beat=$((beat + 1))
			write_progress "$1" "$beat"
		done
	) &
	HEARTBEAT_PID=$!
}

# @description start a new step. writes its name to PROGRESS_FILE for the
#   on-screen display, and keeps the step marked as alive until the next one
#   begins.
# @arg $1 string progress text
# @exitcode 0 always, write errors are swallowed
begin_step() {
	heartbeat_stop
	write_progress "$1" 0
	heartbeat_start "$1"
}

#---------------------------------
#--- checks and swap

# @description check a tree holds a usable norns binary.
# @arg $1 path tree root
# @exitcode 0 if the binary is present, executable, and non-empty
# @exitcode 1 otherwise
verify_norns_tree() {
	[ -x "$1/$NORNS_BINARY" ] && [ -s "$1/$NORNS_BINARY" ]
}

# @description prove the norns binary starts. uses --check flag which exits
#   before jack or hardware come up.
# @arg $1 path tree root
# @exitcode 0 if the binary launches within LAUNCH_TIMEOUT
# @exitcode 1 otherwise
launch_check() {
	timeout "$LAUNCH_TIMEOUT" env LD_BIND_NOW=1 "$1/$NORNS_BINARY" --check >/dev/null 2>&1
}

# @description true when a dir holds at least one entry.
# @arg $1 path dir to test
# @exitcode 0 if the dir is non-empty
# @exitcode 1 otherwise
tree_nonempty() {
	[ -n "$(ls -A "$1" 2>/dev/null)" ]
}

# @description post-swap gate. the tree must verify and the binary must launch.
# @arg $1 path tree root
# @stdout the failed check name (verify or launch), for the abort message
# @exitcode 0 if both checks pass
# @exitcode 1 on the first failure
verify_norns_live() {
	verify_norns_tree "$1" || { echo "verify"; return 1; }
	launch_check "$1" || { echo "launch"; return 1; }
}

# @description move a tree to a path that must not already exist.
# @arg $1 path source
# @arg $2 path destination, must not exist
# @exitcode 0 if the move happened
# @exitcode 1 if the destination is already there, or the move failed
move_tree() {
	[ -e "$2" ] && return 1
	mv "$1" "$2"
}

# @description put the saved tree back after a failed swap.
# @arg $1 path live dir
# @exitcode 0 if there was nothing to restore, or the restore worked
# @exitcode 1 if the saved tree is still sitting at $1.old
restore_tree() {
	[ -d "$1.old" ] || return 0
	$SUDO rm -rf "$1"
	move_tree "$1.old" "$1"
}

# @description replace the live tree at a dir with a new source. any failure
#   rolls back and aborts via fail. a rollback that cannot finish aborts with
#   its own step name.
# @arg $1 path source tree to install
# @arg $2 path live dir to replace
# @arg $3 string step name reported by fail on any error
# @arg $4 function optional stage check, run on $dir.new before the swap
# @arg $5 function optional live check, run on the swapped-in $dir. its stdout
#   names the failed check in the abort message
# @exitcode 0 on a completed swap
# @exitcode 1 aborts via fail on any failure
swap_tree() {
	local src="$1" dir="$2" step="$3" stage_check="${4:-}" live_check="${5:-}"
	local name
	name="$(basename "$dir")"
	begin_step "updating $name"
	$SUDO rm -rf "$dir.old" "$dir.new"
	cp -a "$src" "$dir.new" || { $SUDO rm -rf "$dir.new"; fail "$step" "could not stage $name.new"; }
	if [ -n "$stage_check" ] && ! "$stage_check" "$dir.new"; then
		$SUDO rm -rf "$dir.new"
		fail "$step" "staged $name.new did not validate"
	fi
	if [ -d "$dir" ]; then
		move_tree "$dir" "$dir.old" || { $SUDO rm -rf "$dir.new"; fail "$step" "could not move live $name aside"; }
	fi
	if ! move_tree "$dir.new" "$dir"; then
		restore_tree "$dir" || fail "$step-rollback" "could not move $name.new into place, and $name could NOT be restored from $dir.old"
		fail "$step" "could not move $name.new into place (rolled back)"
	fi
	if [ -n "$live_check" ]; then
		local why
		if ! why="$("$live_check" "$dir")"; then
			[ -n "$why" ] || why="live"
			restore_tree "$dir" || fail "$step-rollback" "post-swap $name $why check failed, and $name could NOT be restored from $dir.old"
			fail "$step" "post-swap $name $why check failed (rolled back)"
		fi
	fi
	$SUDO rm -rf "$dir.old"
}

# @description check each .sha256 in the update dir against its file.
# @noargs
# @stdout sha256sum's per-file OK lines, then the mismatch reason on failure
# @exitcode 0 if every file matches
# @exitcode 1 on the first mismatch or missing .sha256
verify_checksum() {
	(
		cd "$UPDATE_DIR" || { echo "$UPDATE_DIR missing"; exit 1; }
		for sha_file in "$UPDATE_DIR"/*.sha256; do
			[ -f "$sha_file" ] || { echo "no .sha256 file in $UPDATE_DIR"; exit 1; }
			sha256sum -c "$sha_file" || { echo "download does not match $(basename "$sha_file")"; exit 1; }
		done
	)
}

#---------------------------------
#--- usage and mode predicates

# @description print how to run this script.
# @noargs
# @stderr the usage text
usage() {
	{
		echo "norns update script usage:"
		echo "  run from an unpacked release directory: installs that release"
		echo "  run as the installed copy with a release .tgz and its .sha256"
		echo "  waiting in $UPDATE_DIR: verifies, installs, reboots"
		echo "  update.sh download <tgz-url> <sha-url>: fetches a release and its"
		echo "  checksum into $UPDATE_DIR and verifies it"
		echo "  progress is written to $UPDATE_LOG"
		echo "  (SYSTEM>UPDATE downloads the release and runs the same path)"
	} >&2
}

# @description true when a dir looks like an unpacked release.
# @arg $1 path dir to test
# @exitcode 0 if $1 holds a norns or maiden tree
# @exitcode 1 otherwise
is_release_dir() {
	[ -d "$1/norns" ] || [ -d "$1/maiden" ]
}

# @description true when a downloaded release .tgz is waiting in the update dir.
# @noargs
# @exitcode 0 if a .tgz is present
# @exitcode 1 otherwise
is_release_downloaded() {
	local tgz
	for tgz in "$UPDATE_DIR"/*.tgz; do
		[ -f "$tgz" ] && return 0
	done
	return 1
}

# @description true when a dir is inside the update dir.
# @arg $1 path dir to test
# @exitcode 0 if $1 is the update dir or below it
# @exitcode 1 otherwise
is_in_update_dir() {
	case "$1" in
	"$UPDATE_DIR" | "$UPDATE_DIR"/*) return 0 ;;
	*) return 1 ;;
	esac
}


# @description true while the detached updater is still running.
# @noargs
# @exitcode 0 if the update unit is running
# @exitcode 1 otherwise
update_in_progress() {
	case "$(systemctl is-active "$UPDATE_UNIT" 2>/dev/null)" in
	activ* | deactivating | reloading) return 0 ;;
	*) return 1 ;;
	esac
}

#---------------------------------
#--- download subcommand

# @description fetch the release and its .sha256 into the update dir. failures
#   stay on stdout and in the exit code, not in STATUS_FILE.
# @arg $1 string release .tgz url
# @arg $2 string matching .sha256 url
# @stdout wget progress, then a download: ok/failed line
# @exitcode 0 on success
# @exitcode 1 on a fetch failure, or when an update is already running
# @exitcode 2 on a checksum mismatch
do_download() {
	if update_in_progress; then
		echo "download: failed, an update is already running"
		exit 1
	fi
	mkdir -p "$UPDATE_DIR"
	rm -rf "$UPDATE_DIR"/*
	local progress_style=dot:mega
	[ -t 1 ] && progress_style=bar:force
	if ! wget -T 180 --progress="$progress_style" -P "$UPDATE_DIR" "$1" "$2" 2>&1; then
		echo "download: failed"
		exit 1
	fi
	verify_checksum || { echo "download: failed checksum verification"; exit 2; }
	echo "download: ok"
	exit 0
}


#---------------------------------
#--- update mode

# @description copy this script to /tmp and exec the copy.
# @noargs
# @exitcode 1 if staging fails, otherwise this function execs and never returns
stage_update() {
	cp "$0" "$STAGED_COPY" || fail env "could not stage updater to $STAGED_COPY"
	exec /bin/bash "$STAGED_COPY"
}

# @description verify the download, unpack it, run the release's own install,
#   and reboot only into a verified binary.
# @noargs
# @stdout a pointer to UPDATE_LOG before output is redirected there
# @exitcode 0 on success, after requesting a reboot
# @exitcode 1 on failure, without rebooting (or the install script's exit code)
do_update() {
	local release_version install_script install_status
	echo "update: progress in $UPDATE_LOG"
	exec >>"$UPDATE_LOG" 2>&1
	export HOME="$WE_DIR"
	echo "update: starting: $(date)"
	rm -f "$STATUS_FILE" "$PROGRESS_FILE"

	cd "$UPDATE_DIR" || fail env "$UPDATE_DIR missing"

	echo "update: verifying checksum..."
	begin_step "verifying checksum"
	verify_checksum || fail checksum "download did not verify against its .sha256"

	echo "update: unpacking..."
	begin_step "unpacking"
	local -a tgz=("$UPDATE_DIR"/*.tgz)
	if [ "${#tgz[@]}" -ne 1 ] || [ ! -f "${tgz[0]}" ]; then
		fail unpack "expected exactly one release .tgz in $UPDATE_DIR"
	fi
	tar xzf "${tgz[0]}" -C "$UPDATE_DIR/" || fail unpack "unpack failed"

	release_version="$(tar tzf "${tgz[0]}" 2>/dev/null | head -n 1 | sed 's|^\./||' | cut -d/ -f1)"
	[ -n "$release_version" ] || fail missing-script "could not read a version directory from the download"
	echo "update: found release $release_version"

	install_script="$UPDATE_DIR/$release_version/update.sh"
	[ -f "$install_script" ] || fail missing-script "$install_script not found after unpack"

	echo "update: running install ($install_script)..."
	begin_step "installing"
	heartbeat_stop
	if /bin/bash "$install_script"; then
		if verify_norns_tree "$NORNS_DIR" && launch_check "$NORNS_DIR"; then
			echo "update: ok, rebooting: $(date)"
			sync
			$SUDO systemctl reboot
		else
			fail postcheck "post-update binary missing/not launchable, NOT rebooting"
		fi
	else
		install_status=$?
		[ -s "$STATUS_FILE" ] || echo "failed:update-script" > "$STATUS_FILE"
		echo "update: FAILED (exit $install_status), not rebooting, see $UPDATE_LOG"
		exit "$install_status"
	fi
	exit 0
}


#---------------------------------
#--- install mode

# @description install the unpacked release this script sits in.
# @noargs
# @stdout progress lines
# @exitcode 0 on success
# @exitcode 1 if a check or critical step fails
do_install() {
	local version free_kb
	cd "$SCRIPT_DIR" || fail env "cannot cd to the release directory"

	version=$(cat "$WE_DIR/version.txt") || fail disk-image "cannot read $WE_DIR/version.txt"
	case "$version" in
	'' | *[!0-9]*) fail disk-image "no usable version in $WE_DIR/version.txt" ;;
	esac
	[ "$version" -ge "$MIN_DISK_VERSION" ] || fail disk-image "needs a new disk image ($version < $MIN_DISK_VERSION)"

	[ -d norns ] || fail binary-swap "staged norns tree missing"
	verify_norns_tree norns || fail binary-swap "staged norns binary missing/not executable"
	[ -d maiden ] || fail maiden-swap "staged maiden tree missing"

	free_kb=$(df -kP "$WE_DIR" | awk 'NR==2 {print $4}')
	[ "${free_kb:-0}" -ge "$MIN_SWAP_KB" ] || fail disk-space "low disk: ${free_kb}KB free < ${MIN_SWAP_KB}KB needed to stage swap"

	begin_step "installing packages"

	$SUDO cp config/raspi.list "$ROOT/etc/apt/sources.list.d/"

	$SUDO apt-get update && $SUDO apt-get -y install libnng1 libnng-dev
	dpkg -s libnng1 >/dev/null 2>&1 || fail libnng "libnng1 not installed"

	$SUDO systemctl disable norns-matron.service 2>/dev/null
	$SUDO systemctl disable norns-crone.service 2>/dev/null
	$SUDO rm -f "$ROOT/etc/systemd/system/norns-matron.service"
	$SUDO rm -f "$ROOT/etc/systemd/system/norns-crone.service"
	$SUDO cp --remove-destination config/norns-main.service "$ROOT/etc/systemd/system/norns-main.service" || fail units "install norns-main.service failed"
	$SUDO cp --remove-destination config/norns-sclang.service "$ROOT/etc/systemd/system/norns-sclang.service" || fail units "install norns-sclang.service failed"
	$SUDO cp --remove-destination config/norns.target "$ROOT/etc/systemd/system/norns.target" || fail units "install norns.target failed"
	$SUDO systemctl enable norns-main.service || fail units "enable norns-main.service failed"
	$SUDO systemctl enable norns-sclang.service || fail units "enable norns-sclang.service failed"

	$SUDO cp --remove-destination config/norns-watcher.service "$ROOT/etc/systemd/system/norns-watcher.service" || fail units "install norns-watcher.service failed"
	$SUDO systemctl enable norns-watcher || fail units "enable norns-watcher failed"

	$SUDO cp --remove-destination config/norns-jack.service "$ROOT/etc/systemd/system/norns-jack.service" || fail units "install norns-jack.service failed"

	swap_tree maiden "$MAIDEN_DIR" maiden-swap tree_nonempty

	$SUDO rm -rf "$WE_DIR/bin/maiden-repl"
	$SUDO cp -a norns/build/maiden-repl/maiden-repl "$WE_DIR/bin/" || fail maiden-swap "maiden-repl install failed"

	swap_tree norns "$NORNS_DIR" binary-swap verify_norns_tree verify_norns_live
	begin_step "finishing"

	cp version.txt "$WE_DIR/"
	cp changelog.txt "$WE_DIR/"

	$SUDO apt -y remove rsyslog
	$SUDO cp config/logrotate.conf "$ROOT/etc/"
	$SUDO cp config/journald.conf "$ROOT/etc/systemd/"
	$SUDO rm -rf "$ROOT/var/log/journal"
	$SUDO rm -rf "$ROOT/var/log/daemon.log"
	$SUDO rm -rf "$ROOT/var/log/user.log"

	$SUDO systemctl disable hciuart

	find "$WE_DIR/dust" -name .DS_Store -delete
	find "$WE_DIR/dust" -name ._.DS_Store -delete

	# set alsa mixer
	amixer --device hw:sndrpimonome set Master 100% on
	$SUDO alsactl store

	# change boot/cmdline for screen
	$SUDO sed -e '/dtoverlay=ssd1322-spi/ s/^#*/#/' -i "$ROOT/boot/config.txt"
	$SUDO sed -e '/spidev.bufsiz/! s/$/ spidev.bufsiz=8192/' -i "$ROOT/boot/cmdline.txt"

	# install packages
	$SUDO dpkg -i package/*.deb

	# clean slate
	rm -f "$WE_DIR/matronrc.lua"

	# get common audio if not present
	if [ ! -d "$WE_DIR/dust/audio/common" ]; then
		echo "common audio does not exist, downloading"
		(
			cd "$WE_DIR/dust/audio" &&
				wget "$DUST_AUDIO_COMMON_URL" &&
				tar xzvf "$DUST_AUDIO_COMMON_TGZ" &&
				rm "$DUST_AUDIO_COMMON_TGZ"
		)
	fi

	# update libmonome
	if [ -d package/libmonome ]; then
		( cd package/libmonome && ./waf configure && $SUDO ./waf install )
	fi

	# maiden project setups
	( cd "$MAIDEN_DIR" && ./project-setup.sh )

	# cleanup
	rm -rf "$UPDATE_DIR"/*
	echo "update: complete"
	exit 0
}


#---------------------------------
#--- mode routing

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly SCRIPT_ABS_PATH="$SCRIPT_DIR/$(basename "$0")"

trap heartbeat_stop EXIT

# @description true when a release is downloaded and ready to stage.
# @noargs
# @exitcode 0 if a downloaded release is ready to stage
# @exitcode 1 otherwise
should_stage_update() {
	is_release_downloaded && [ "$SCRIPT_ABS_PATH" != "$STAGED_COPY" ] &&
		! is_release_dir "$SCRIPT_DIR" && ! is_in_update_dir "$SCRIPT_DIR"
}

if [ "${1:-}" = "download" ] && [ $# -eq 3 ]; then
	do_download "$2" "$3"
elif [ $# -eq 0 ] && should_stage_update; then
	stage_update
# the exec restarts the script from the top, routing the copy in to do the update
elif [ $# -eq 0 ] && [ "$SCRIPT_ABS_PATH" = "$STAGED_COPY" ]; then
	do_update
elif [ $# -eq 0 ] && is_release_dir "$SCRIPT_DIR"; then
	do_install
else
	usage
	exit 1
fi
