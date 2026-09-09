#!/usr/bin/env bash
# dry-run tests for update/update.sh. no device, no root, no network.
#
# run: bash update/test-update.sh [case-glob]
#
# each case builds a throwaway file tree and runs the real update.sh against it.

set -u
UPDATE_SH="$(cd "$(dirname "$0")" && pwd)/update.sh"

if ! bash -n "$UPDATE_SH"; then
	echo "FAIL: bash -n update.sh"
	exit 1
fi

TEST_VERSION=260616
TEST_LIVE_VERSION=260000
NORNS_BINARY=build/norns/norns

fails=0
cases=0

ONLY="${1:-}"

trap 'rm -rf "${TEST_ROOT:-}"' INT TERM

export NORNS_UPDATE_HEARTBEAT_SECS=1

TGZ_URL=http://example.com/bundle.tgz
SHA_URL=http://example.com/bundle.sha256

# update.sh builds every absolute path from its ROOT prefix. this catches a
# hardcoded system path.
stray="$(sed 's|\$ROOT/|$ROOT_|g' "$UPDATE_SH" | grep -nE '(/(home/we|etc/|var/log|boot/|tmp/norns)|~/)' | grep -vE '^[0-9]+:[[:space:]]*#' || true)"
if [ -n "$stray" ]; then
	echo "FAIL: update.sh has absolute paths not built from ROOT:"
	echo "$stray"
	exit 1
fi

#---------------------------------
#--- file builders

write_binary() {
	mkdir -p "$1/$(dirname "$NORNS_BINARY")"
	printf '%s' "$2" > "$1/$NORNS_BINARY"
	chmod +x "$1/$NORNS_BINARY"
}

write_ok_script() {
	printf '#!/bin/sh\nexit 0\n' > "$1"
	chmod +x "$1"
}

make_bundle_with_script() {
	rm -rf "$TEST_ROOT/build"
	mkdir -p "$TEST_ROOT/build/$TEST_VERSION"
	printf '%s' "$1" > "$TEST_ROOT/build/$TEST_VERSION/update.sh"
	tar czf "$TEST_ROOT/home/we/update/bundle.tgz" -C "$TEST_ROOT/build" "$TEST_VERSION"
}

make_bundle_without_script() {
	rm -rf "$TEST_ROOT/build"
	mkdir -p "$TEST_ROOT/build/$TEST_VERSION"
	printf 'x\n' > "$TEST_ROOT/build/$TEST_VERSION/other"
	tar czf "$TEST_ROOT/home/we/update/bundle.tgz" -C "$TEST_ROOT/build" "$TEST_VERSION"
}

place_installed_binary() { write_binary "$TEST_ROOT/home/we/norns" NEW; }

write_bundle_sha() { (cd "$TEST_ROOT/home/we/update" && sha256sum bundle.tgz > bundle.tgz.sha256); }
write_bad_bundle_sha() { printf '%064d  bundle.tgz\n' 0 > "$TEST_ROOT/home/we/update/bundle.tgz.sha256"; }

#---------------------------------
#--- post-run checks

check_update_progress() { [ -s "$TEST_ROOT/tmp/norns-update-progress" ]; }

check_download_result() {
	[ -f "$TEST_ROOT/home/we/update/bundle.tgz" ] || return 1
	[ -f "$TEST_ROOT/home/we/update/bundle.tgz.sha256" ] || return 1
	[ ! -e "$TEST_ROOT/home/we/update/junk" ]
}

check_download_untouched() { [ -e "$TEST_ROOT/home/we/update/junk" ]; }

check_progress_beat() {
	local beat
	beat="$(sed -n 2p "$TEST_ROOT/progress-during-apt" 2>/dev/null)"
	[ -n "$beat" ] && [ "$beat" -gt 0 ] 2>/dev/null
}

check_install_complete() {
	[ -f "$TEST_ROOT/home/we/maiden/index.html" ] || return 1
	[ -x "$TEST_ROOT/home/we/bin/maiden-repl" ] || return 1
	[ -f "$TEST_ROOT/etc/systemd/system/norns-main.service" ] || return 1
	[ "$(cat "$TEST_ROOT/home/we/version.txt")" = "$TEST_VERSION" ]
}

check_common_audio_cwd() { grep -q "^wget .* | $TEST_ROOT/home/we/dust/audio\$" "$TEST_ROOT/calls.log"; }
check_no_wget() { ! grep -q '^wget' "$TEST_ROOT/calls.log" 2>/dev/null; }
check_expected_one_tgz() { grep -q "exactly one" "$TEST_ROOT/tmp/norns-update.log"; }

#---------------------------------
#--- fake commands

INSTALL_OK=$'#!/bin/sh\nexit 0'

shim() {
	cat > "$TEST_ROOT/bin/$1"
	chmod +x "$TEST_ROOT/bin/$1"
}

shim_fails_on() {
	local name="$1" fault="$2" only_arg="${3:-}"
	if [ -n "$only_arg" ]; then
		shim "$name" <<EOF
#!/usr/bin/env bash
if [ "\$1" = "$only_arg" ]; then case "\${FAULT:-}" in $fault) exit 1 ;; esac; fi
exit 0
EOF
	else
		shim "$name" <<EOF
#!/usr/bin/env bash
case "\${FAULT:-}" in $fault) exit 1 ;; esac
exit 0
EOF
	fi
}

shim_wraps_real() {
	local name="$1" fault="$2" glob="$3"
	shim "$name" <<EOF
#!/usr/bin/env bash
if [ "\${FAULT:-}" = "$fault" ]; then
	for a in "\$@"; do case "\$a" in $glob) exit 1 ;; esac; done
fi
exec /bin/$name "\$@"
EOF
}

shim_ok() {
	local name
	for name in "$@"; do
		shim "$name" <<EOF
#!/usr/bin/env bash
printf '%s %s | %s\n' "\$(basename "\$0")" "\$*" "\$PWD" >> "$TEST_ROOT/calls.log"
exit 0
EOF
	done
}

#---------------------------------
#--- install fixtures

make_release_fixture() {
	RELEASE_DIR="$TEST_ROOT/home/we/update/$TEST_VERSION"
	mkdir -p "$RELEASE_DIR/norns/build/maiden-repl" "$RELEASE_DIR/maiden" "$RELEASE_DIR/config"
	write_binary "$RELEASE_DIR/norns" NEW
	write_ok_script "$RELEASE_DIR/norns/build/maiden-repl/maiden-repl"
	write_ok_script "$RELEASE_DIR/maiden/project-setup.sh"
	printf 'maiden\n' > "$RELEASE_DIR/maiden/index.html"
	local f
	for f in journald.conf logrotate.conf norns-jack.service norns-main.service \
		norns-sclang.service norns-watcher.service norns.target raspi.list; do
		printf 'stub\n' > "$RELEASE_DIR/config/$f"
	done
	printf '%s\n' "$TEST_VERSION" > "$RELEASE_DIR/version.txt"
	printf 'changelog\n' > "$RELEASE_DIR/changelog.txt"
}

make_live_system() {
	mkdir -p "$TEST_ROOT/home/we/maiden" "$TEST_ROOT/home/we/bin" "$TEST_ROOT/home/we/dust/audio/common"
	write_binary "$TEST_ROOT/home/we/norns" OLD
	printf '%s\n' "$TEST_LIVE_VERSION" > "$TEST_ROOT/home/we/version.txt"
	mkdir -p "$TEST_ROOT/etc/apt/sources.list.d" "$TEST_ROOT/etc/systemd/system" "$TEST_ROOT/var/log" "$TEST_ROOT/boot"
	printf '#stub\n' > "$TEST_ROOT/boot/config.txt"
	printf 'stub\n' > "$TEST_ROOT/boot/cmdline.txt"
}

make_install_shims() {
	mkdir -p "$TEST_ROOT/bin"
	shim df <<'EOF'
#!/usr/bin/env bash
echo "Filesystem 1K-blocks Used Available Capacity Mounted"
if [ "${FAULT:-}" = "disk" ]; then echo "stub 100 90 1000 90% /"; else echo "stub 100 10 99999999 1% /"; fi
EOF
	shim apt-get <<EOF
#!/usr/bin/env bash
if [ "\${FAULT:-}" = "slow" ]; then
	sleep 3
	cp "$TEST_ROOT/tmp/norns-update-progress" "$TEST_ROOT/progress-during-apt" 2>/dev/null
fi
exit 0
EOF
	shim_fails_on dpkg libnng -s
	shim_fails_on systemctl units enable
	shim_fails_on timeout 'launch|launch-restore'
	shim_wraps_real rm launch-restore '*/norns'
	shim_ok apt amixer alsactl wget
}

setup_install_test() {
	TEST_ROOT="$(mktemp -d)"
	make_release_fixture
	make_live_system
	make_install_shims
	mkdir -p "$TEST_ROOT/tmp"
	cp "$UPDATE_SH" "$RELEASE_DIR/update.sh"
	STATUS_FILE="$TEST_ROOT/tmp/norns-update-status"
}

remove_release_binary() { rm -f "$RELEASE_DIR/norns/$NORNS_BINARY"; }
empty_release_maiden()  { rm -rf "$RELEASE_DIR"/maiden/*; }
old_disk_image()        { printf '220305\n' > "$TEST_ROOT/home/we/version.txt"; }
garbage_version()       { printf '\n' > "$TEST_ROOT/home/we/version.txt"; }
remove_common_audio()   { rm -rf "$TEST_ROOT/home/we/dust/audio/common"; }
remove_dust_audio()     { rm -rf "$TEST_ROOT/home/we/dust/audio"; }

#---------------------------------
#--- update fixtures

make_update_shims() {
	mkdir -p "$TEST_ROOT/bin"
	shim systemctl <<EOF
#!/usr/bin/env bash
if [ "\$1" = "reboot" ]; then : > "$REBOOT_FLAG"; fi
if [ "\$1" = "is-active" ]; then
	if [ "\${FAULT:-}" = "busy" ]; then echo activating; exit 0; fi
	echo inactive
	exit 3
fi
exit 0
EOF
	shim wget <<EOF
#!/usr/bin/env bash
[ "\${FAULT:-}" = "wget" ] && exit 1
dest=.
while [ \$# -gt 0 ]; do
	if [ "\$1" = "-P" ]; then dest="\$2"; shift; fi
	shift
done
cp "$TEST_ROOT/fixture/"* "\$dest/"
EOF
	shim_fails_on timeout launch
	shim_wraps_real cp stage '*norns-update-staged*'
}

setup_update_test() {
	TEST_ROOT="$(mktemp -d)"
	mkdir -p "$TEST_ROOT/home/we/update" "$TEST_ROOT/tmp"
	REBOOT_FLAG="$TEST_ROOT/reboot.flag"
	STATUS_FILE="$TEST_ROOT/tmp/norns-update-status"
	make_update_shims
}

stage_ready() {
	setup_update_test
	make_bundle_with_script "$INSTALL_OK"
	write_bundle_sha
	place_installed_binary
}

stage_no_binary() {
	setup_update_test
	make_bundle_with_script "$INSTALL_OK"
	write_bundle_sha
}

stage_bad_sha() {
	setup_update_test
	make_bundle_with_script "$INSTALL_OK"
	write_bad_bundle_sha
	place_installed_binary
}

stage_no_sha() {
	setup_update_test
	make_bundle_with_script "$INSTALL_OK"
	place_installed_binary
}

stage_not_tarball() {
	setup_update_test
	printf 'not a tarball\n' > "$TEST_ROOT/home/we/update/bundle.tgz"
	write_bundle_sha
}

stage_no_script() {
	setup_update_test
	make_bundle_without_script
	write_bundle_sha
}

stage_bare() { setup_update_test; }
stage_install_fail() {
	setup_update_test
	make_bundle_with_script "#!/bin/sh
echo failed:units > $STATUS_FILE
exit 1"
	write_bundle_sha
	place_installed_binary
}
stage_debris() {
	stage_ready
	cp "$UPDATE_SH" "$TEST_ROOT/home/we/update/update.sh"
}

stage_orphan_copy() {
	setup_update_test
	cp "$UPDATE_SH" "$TEST_ROOT/tmp/norns-update-staged.sh"
	rm -rf "$TEST_ROOT/home/we/update"
}
stage_download() {
	setup_update_test
	make_bundle_with_script "$INSTALL_OK"
	write_bundle_sha
	mkdir -p "$TEST_ROOT/fixture"
	mv "$TEST_ROOT/home/we/update/bundle.tgz" "$TEST_ROOT/home/we/update/bundle.tgz.sha256" "$TEST_ROOT/fixture/"
	printf 'debris\n' > "$TEST_ROOT/home/we/update/junk"
}
stage_download_bad_sha() {
	stage_download
	printf '%064d  bundle.tgz\n' 0 > "$TEST_ROOT/fixture/bundle.tgz.sha256"
}

stage_two_tgz() {
	stage_ready
	cp "$TEST_ROOT/home/we/update/bundle.tgz" "$TEST_ROOT/home/we/update/extra.tgz"
}

#---------------------------------
#--- case runners

run_case() {
	local name="$1" fault="$2" want_exit_code="$3" want_status="$4" want_live="$5" pre_hook="${6:-}"
	local allow_leftover="${7:-}" check="${8:-}"
	case "$name" in ${ONLY:-*}) ;; *) return ;; esac
	cases=$((cases + 1))
	setup_install_test
	[ -n "$pre_hook" ] && "$pre_hook"

	PATH="$TEST_ROOT/bin:$PATH" HOME="$TEST_ROOT/home/we" FAULT="$fault" \
		NORNS_UPDATE_ROOT="$TEST_ROOT" NORNS_UPDATE_SUDO= \
		bash "$RELEASE_DIR/update.sh" >"$TEST_ROOT/out.log" 2>&1
	local exit_code=$?
	local ok=1

	if [ "$exit_code" != "$want_exit_code" ]; then
		echo "  [$name] FAIL exit code: got $exit_code want $want_exit_code"; ok=0
	fi

	local live; live="$(cat "$TEST_ROOT/home/we/norns/$NORNS_BINARY" 2>/dev/null || echo MISSING)"
	if [ "$live" != "$want_live" ]; then
		echo "  [$name] FAIL installed binary: got '$live' want '$want_live'"; ok=0
	fi

	if [ -z "$want_status" ]; then
		if [ -e "$STATUS_FILE" ]; then echo "  [$name] FAIL: status file present ($(cat "$STATUS_FILE")) want none"; ok=0; fi
	else
		local got; got="$(cat "$STATUS_FILE" 2>/dev/null || echo NONE)"
		if [ "$got" != "$want_status" ]; then echo "  [$name] FAIL status: got '$got' want '$want_status'"; ok=0; fi
	fi

	for leftover in norns.old norns.new maiden.old maiden.new; do
		case " $allow_leftover " in *" $leftover "*) continue ;; esac
		if [ -e "$TEST_ROOT/home/we/$leftover" ]; then echo "  [$name] FAIL: leftover $leftover present"; ok=0; fi
	done

	if [ -n "$check" ] && ! "$check"; then
		echo "  [$name] FAIL: post-run check $check"; ok=0
	fi

	if [ "$ok" = 1 ]; then
		echo "  [$name] ok"
		rm -rf "$TEST_ROOT"
	else
		fails=$((fails + 1))
		tail -n 25 "$TEST_ROOT/out.log" 2>/dev/null | sed 's/^/  | /'
		echo "  [$name] tree kept at $TEST_ROOT"
	fi
}

run_update() {
	local name="$1" fixture="$2" want_exit_code="$3" want_status="$4" want_reboot="$5"
	shift 5
	case "$name" in ${ONLY:-*}) ;; *) return ;; esac
	cases=$((cases + 1))
	local fault="" want="" check="" run=""
	local -a passthru=()
	while [ $# -gt 0 ]; do
		case "$1" in
		fault=*) fault="${1#fault=}" ;;
		want=*) want="${1#want=}" ;;
		check=*) check="${1#check=}" ;;
		run=*) run="${1#run=}" ;;
		--) shift; passthru=("$@"); break ;;
		*) echo "  [$name] FAIL: unknown run_update token '$1'"; fails=$((fails + 1)); return ;;
		esac
		shift
	done

	"$fixture"
	local run_script="$UPDATE_SH"
	[ "$run" = update-dir ] && run_script="$TEST_ROOT/home/we/update/update.sh"
	[ "$run" = staged ] && run_script="$TEST_ROOT/tmp/norns-update-staged.sh"

	PATH="$TEST_ROOT/bin:$PATH" FAULT="$fault" \
		NORNS_UPDATE_ROOT="$TEST_ROOT" NORNS_UPDATE_SUDO= \
		bash "$run_script" "${passthru[@]}" >"$TEST_ROOT/run.log" 2>&1
	local exit_code=$? ok=1
	[ "$exit_code" = "$want_exit_code" ] || { echo "  [$name] FAIL exit code: got $exit_code want $want_exit_code"; ok=0; }
	if [ -z "$want_status" ]; then
		[ -e "$STATUS_FILE" ] && { echo "  [$name] FAIL: status file present ($(cat "$STATUS_FILE"))"; ok=0; }
	else
		local got; got="$(cat "$STATUS_FILE" 2>/dev/null || echo NONE)"
		[ "$got" = "$want_status" ] || { echo "  [$name] FAIL status: got '$got' want '$want_status'"; ok=0; }
	fi
	if [ "$want_reboot" = yes ]; then
		[ -e "$REBOOT_FLAG" ] || { echo "  [$name] FAIL: expected reboot, none recorded"; ok=0; }
	else
		[ -e "$REBOOT_FLAG" ] && { echo "  [$name] FAIL: reboot called when it must not be"; ok=0; }
	fi
	if [ -n "$want" ] && ! grep -qi "$want" "$TEST_ROOT/run.log"; then
		echo "  [$name] FAIL: output does not mention '$want'"; ok=0
	fi
	if [ -n "$check" ] && ! "$check"; then
		echo "  [$name] FAIL: post-run check $check"; ok=0
	fi
	if [ "$ok" = 1 ]; then
		echo "  [$name] ok"
		rm -rf "$TEST_ROOT"
	else
		fails=$((fails + 1))
		tail -n 25 "$TEST_ROOT/run.log" 2>/dev/null | sed 's/^/  | /'
		echo "  [$name] tree kept at $TEST_ROOT"
	fi
}

#---------------------------------
#--- cases

echo "install step only, run from inside an unpacked release:"
run_case good             ""        0 ""                   NEW "" "" check_install_complete
run_case release-missing  ""        1 "failed:binary-swap" OLD remove_release_binary
run_case libnng           libnng    1 "failed:libnng"      OLD
run_case units            units     1 "failed:units"       OLD
run_case maiden-empty     ""        1 "failed:maiden-swap" OLD empty_release_maiden
run_case disk-space       disk      1 "failed:disk-space"  OLD
run_case old-disk-image   ""        1 "failed:disk-image"  OLD old_disk_image
run_case bad-version      ""        1 "failed:disk-image"  OLD garbage_version
run_case launch           launch    1 "failed:binary-swap" OLD
run_case launch-rollback  launch-restore 1 "failed:binary-swap-rollback" NEW "" norns.old
run_case slow-step        slow      0 ""                   NEW "" "" check_progress_beat
run_case common-audio     ""        0 ""                   NEW remove_common_audio "" check_common_audio_cwd
run_case no-dust-audio    ""        0 ""                   NEW remove_dust_audio "" check_no_wget

echo ""
echo "whole update, from verifying the download to the reboot:"
run_update update-good           stage_ready        0 ""                      yes  check=check_update_progress
run_update update-stage-fail     stage_ready        1 "failed:env"            no   fault=stage
run_update update-dir-missing    stage_orphan_copy  1 "failed:env"            no   run=staged
run_update update-postcheck      stage_no_binary    1 "failed:postcheck"      no
run_update update-launch         stage_ready        1 "failed:postcheck"      no   fault=launch
run_update update-install-fail   stage_install_fail 1 "failed:units"          no
run_update update-checksum       stage_bad_sha      1 "failed:checksum"       no
run_update update-no-sha         stage_no_sha       1 "failed:checksum"       no
run_update update-unpack         stage_not_tarball  1 "failed:unpack"         no
run_update update-missing-script stage_no_script    1 "failed:missing-script" no
run_update update-two-tgz        stage_two_tgz      1 "failed:unpack"         no   check=check_expected_one_tgz
run_update update-usage          stage_bare         1 ""                      no   want=usage
run_update update-stray-arg      stage_ready        1 ""                      no   want=usage -- stray
run_update update-debris         stage_debris       1 ""                      no   want=usage run=update-dir

echo ""
echo "download subcommand:"
run_update download-good           stage_download         0 "" no  check=check_download_result want="download: ok" -- download "$TGZ_URL" "$SHA_URL"
run_update download-wget-fail      stage_download         1 "" no  fault=wget -- download "$TGZ_URL" "$SHA_URL"
run_update download-bad-sha        stage_download_bad_sha 2 "" no  want=checksum -- download "$TGZ_URL" "$SHA_URL"
run_update download-missing-arg    stage_bare             1 "" no  want=usage -- download "$TGZ_URL"
run_update download-busy           stage_download         1 "" no  fault=busy want="already running" check=check_download_untouched -- download "$TGZ_URL" "$SHA_URL"

echo ""
if [ "$fails" = 0 ]; then
	echo "PASS: $cases/$cases cases"
	exit 0
else
	echo "FAIL: $fails/$cases cases failed"
	exit 1
fi
