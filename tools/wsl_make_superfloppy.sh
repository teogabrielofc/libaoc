#!/bin/sh
set -eu

if [ "$#" -lt 3 ]; then
    echo "usage: $0 <sudo_password> <device> <repo_root_windows_path>" >&2
    exit 2
fi

PW="$1"
DEV="$2"
REPO_WIN="$3"

repo_wsl() {
    printf '%s' "$1" | sed 's#\\#/#g; s#^C:#/mnt/c#'
}

REPO_WSL="$(repo_wsl "$REPO_WIN")"
MNT="/mnt/superfloppy_aoc"

run_sudo() {
    printf '%s\n' "$PW" | sudo -S "$@"
}

run_sudo umount "$DEV" 2>/dev/null || true
run_sudo mkdir -p "$MNT"
run_sudo umount "$MNT" 2>/dev/null || true
run_sudo mkfs.vfat -F 32 -I -n "AQUELE UM" "$DEV"
run_sudo mount -t vfat "$DEV" "$MNT"
run_sudo cp "$REPO_WSL/payloads/media_sub_probe/psb_stage1_core/PC00_BASELINE.avi" "$MNT/"
run_sudo cp "$REPO_WSL/payloads/media_sub_probe/psb_stage1_core/PC00_BASELINE.psb" "$MNT/"
run_sudo cp "$REPO_WSL/payloads/media_sub_probe/psb_stage1_core/PC10_RA_NULL_264.avi" "$MNT/"
run_sudo cp "$REPO_WSL/payloads/media_sub_probe/psb_stage1_core/PC10_RA_NULL_264.psb" "$MNT/"
sync
ls -l "$MNT"
run_sudo umount "$MNT"
blkid "$DEV" || true
