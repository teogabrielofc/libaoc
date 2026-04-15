#!/bin/sh
set -u

CORELOG=/etc/core/launch_doom.log
USBLOG=/mnt/doom/launch_doom_usb.log
ROOTLOG=/mnt/launch_doom.log
TOUCH=/mnt/doom/launcher_touched.txt

safe_append() {
    echo "$1" >> "$2" 2>/dev/null || true
}

log_line() {
    safe_append "$1" "$CORELOG"
    safe_append "$1" "$USBLOG"
    safe_append "$1" "$ROOTLOG"
    sync 2>/dev/null || true
}

mkdir -p /etc/core 2>/dev/null || true
rm -f "$CORELOG" "$USBLOG" "$ROOTLOG" "$TOUCH" \
  /etc/core/doom.log /etc/core/doom.ok /etc/core/doom.fail \
  /etc/core/dfbprobe.log /etc/core/dfbprobe.ok /etc/core/dfbprobe.fail \
  /etc/core/hidprobe.log /etc/core/hidprobe.ok /etc/core/hidprobe.fail \
  /etc/core/pthreadcreateprobe.log /etc/core/pthreadcreateprobe.ok /etc/core/pthreadcreateprobe.fail \
  /etc/core/pthreaddlcreateprobe.log /etc/core/pthreaddlcreateprobe.ok /etc/core/pthreaddlcreateprobe.fail \
  2>/dev/null || true

safe_append "launcher touched" "$TOUCH"
sync 2>/dev/null || true

log_line "launch: start"
log_line "launch: usb log active"

if chmod +x /mnt/doom/doom 2>/dev/null; then
    log_line "launch: chmod doom ok"
else
    log_line "launch: chmod doom failed"
fi

cd /mnt/doom
log_line "launch: cwd /mnt/doom"
export LD_LIBRARY_PATH="/lib:/usr/lib"
export DOOMWADDIR="/mnt/doom"
export AOC_FB_PAGES="${AOC_FB_PAGES:-1}"
export AOC_FB_FULL_REFRESH_EVERY="${AOC_FB_FULL_REFRESH_EVERY:-0}"
export AOC_INPUT_DEBUG="${AOC_INPUT_DEBUG:-0}"
log_line "launch: DOOMWADDIR=$DOOMWADDIR"
log_line "launch: AOC_FB_PAGES=$AOC_FB_PAGES"
log_line "launch: AOC_FB_FULL_REFRESH_EVERY=$AOC_FB_FULL_REFRESH_EVERY"
log_line "launch: AOC_INPUT_DEBUG=$AOC_INPUT_DEBUG"

if "/mnt/doom/doom" -iwad /mnt/doom/doom1.wad -nosound -nomusic -nomonsters -warp 1 1 >> "$USBLOG" 2>&1; then
    log_line "launch: doom exit 0"
    exit 0
fi

status=$?
log_line "launch: doom exit $status"
exit "$status"
