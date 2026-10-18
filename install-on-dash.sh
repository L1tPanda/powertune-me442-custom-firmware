#!/bin/sh
# Build the custom PowerTune app on the dash and install it, keeping the original for rollback.
# Run on the dash:  sh /home/pi/custom-firmware/install-on-dash.sh      (log: /home/pi/custom-build.log)
#
# - patches go onto a git branch "custom" in /home/pi/src (the official checkout stays on its branch)
# - builds in /home/pi/build-custom, NOT /home/pi/build (powertune-update.sh would install that on boot)
# - only replaces /opt/PowerTune/PowertuneQMLGui after a successful build; original kept as .orig
# - takes effect after a reboot
set -e
SRC=/home/pi/src
PATCHES=/home/pi/custom-firmware/patches
BUILD=/home/pi/build-custom
APP=/opt/PowerTune/PowertuneQMLGui
GIT="git -c user.name=custom-firmware -c user.email=custom@localhost"

log() { echo "[$(date +%H:%M:%S)] $*"; }

cd "$SRC"
if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    log "STOP: $SRC has uncommitted changes - not touching it."; git status --short; exit 1
fi
BASE=$(git rev-parse --abbrev-ref HEAD)
if [ "$BASE" != "custom" ]; then
    log "source on branch $BASE ($(git rev-parse --short HEAD)); creating branch custom"
    git checkout -B custom
fi
for p in "$PATCHES"/*.patch; do
    name=$(basename "$p")
    if git log --format=%s | grep -qxF "$name"; then
        log "already applied: $name"
        continue
    fi
    git apply --check "$p" || { log "STOP: $name does not apply"; exit 1; }
    git apply "$p"
    $GIT commit -qam "$name"
    log "applied: $name"
done

log "building in $BUILD (several minutes; the dash keeps running meanwhile)"
rm -rf "$BUILD"; mkdir -p "$BUILD"; cd "$BUILD"
qmake "$SRC"
nice make -j4
[ -x "$BUILD/PowertuneQMLGui" ] || { log "STOP: build produced no app - nothing installed"; exit 1; }

if [ ! -f "$APP.orig" ]; then
    cp -p "$APP" "$APP.orig"
    log "saved original app as $APP.orig"
fi
cp "$BUILD/PowertuneQMLGui" "$APP.new"
mv "$APP.new" "$APP"
log "INSTALLED custom app. Reboot the dash to start it:  reboot"
