import argparse
import ctypes
import math
import msvcrt
import os
import random
import struct
import sys


SECTOR_SIZE = 512
IOCTL_DISK_GET_LENGTH_INFO = 0x7405C


def choose_sectors_per_cluster(total_sectors: int) -> int:
    mib = (total_sectors * SECTOR_SIZE) // (1024 * 1024)
    if mib < 260:
        return 1
    if mib < 8192:
        return 8
    if mib < 16384:
        return 16
    if mib < 32768:
        return 32
    return 64


def compute_fat_layout(total_sectors: int):
    reserved = 32
    fats = 2
    spc = choose_sectors_per_cluster(total_sectors)

    while True:
        data_sectors_guess = total_sectors - reserved
        clusters_guess = data_sectors_guess // spc
        fat_sectors = math.ceil(((clusters_guess + 2) * 4) / SECTOR_SIZE)
        data_sectors = total_sectors - reserved - fats * fat_sectors
        cluster_count = data_sectors // spc
        fat_sectors_2 = math.ceil(((cluster_count + 2) * 4) / SECTOR_SIZE)
        if fat_sectors_2 == fat_sectors:
            break
        fat_sectors = fat_sectors_2

    if cluster_count <= 65525 or cluster_count >= 4177918:
        raise ValueError(f"Computed invalid FAT32 cluster count: {cluster_count}")

    return {
        "reserved": reserved,
        "fats": fats,
        "spc": spc,
        "fat_sectors": fat_sectors,
        "cluster_count": cluster_count,
        "data_start": reserved + fats * fat_sectors,
    }


def build_boot_sector(total_sectors: int, label: str, layout: dict) -> bytes:
    boot = bytearray(SECTOR_SIZE)
    boot[0:3] = b"\xEB\x58\x90"
    boot[3:11] = b"MSWIN4.1"
    struct.pack_into("<H", boot, 11, SECTOR_SIZE)
    boot[13] = layout["spc"]
    struct.pack_into("<H", boot, 14, layout["reserved"])
    boot[16] = layout["fats"]
    struct.pack_into("<H", boot, 17, 0)
    struct.pack_into("<H", boot, 19, 0)
    boot[21] = 0xF8
    struct.pack_into("<H", boot, 22, 0)
    struct.pack_into("<H", boot, 24, 63)
    struct.pack_into("<H", boot, 26, 255)
    struct.pack_into("<I", boot, 28, 0)
    struct.pack_into("<I", boot, 32, total_sectors)
    struct.pack_into("<I", boot, 36, layout["fat_sectors"])
    struct.pack_into("<H", boot, 40, 0)
    struct.pack_into("<H", boot, 42, 0)
    struct.pack_into("<I", boot, 44, 2)
    struct.pack_into("<H", boot, 48, 1)
    struct.pack_into("<H", boot, 50, 6)
    boot[64] = 0x80
    boot[66] = 0x29
    struct.pack_into("<I", boot, 67, random.getrandbits(32))
    boot[71:82] = label.encode("ascii", "replace").upper().ljust(11, b" ")[:11]
    boot[82:90] = b"FAT32   "
    boot[510:512] = b"\x55\xAA"
    return bytes(boot)


def build_fsinfo(cluster_count: int) -> bytes:
    fsinfo = bytearray(SECTOR_SIZE)
    struct.pack_into("<I", fsinfo, 0, 0x41615252)
    struct.pack_into("<I", fsinfo, 484, 0x61417272)
    struct.pack_into("<I", fsinfo, 488, max(cluster_count - 1, 0))
    struct.pack_into("<I", fsinfo, 492, 3)
    fsinfo[508:512] = b"\x00\x00\x55\xAA"
    return bytes(fsinfo)


def build_fat_sector() -> bytes:
    fat = bytearray(SECTOR_SIZE)
    struct.pack_into("<I", fat, 0, 0x0FFFFFF8)
    struct.pack_into("<I", fat, 4, 0xFFFFFFFF)
    struct.pack_into("<I", fat, 8, 0x0FFFFFFF)
    return bytes(fat)


def write_all(handle, offset: int, data: bytes):
    handle.seek(offset)
    handle.write(data)


def get_windows_disk_size(handle) -> int:
    class GET_LENGTH_INFORMATION(ctypes.Structure):
        _fields_ = [("Length", ctypes.c_longlong)]

    bytes_returned = ctypes.c_ulong(0)
    outbuf = GET_LENGTH_INFORMATION()
    ok = ctypes.windll.kernel32.DeviceIoControl(
        msvcrt.get_osfhandle(handle.fileno()),
        IOCTL_DISK_GET_LENGTH_INFO,
        None,
        0,
        ctypes.byref(outbuf),
        ctypes.sizeof(outbuf),
        ctypes.byref(bytes_returned),
        None,
    )
    if not ok:
        raise ctypes.WinError()
    return int(outbuf.Length)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--physical-drive", required=True, help=r"Example: \\\\.\\PhysicalDrive1")
    parser.add_argument("--label", default="AQUELE UM")
    parser.add_argument("--zero-mib", type=int, default=8)
    args = parser.parse_args()

    label = args.label.upper()[:11]

    with open(args.physical_drive, "r+b", buffering=0) as f:
        if os.name == "nt":
            size_bytes = get_windows_disk_size(f)
        else:
            f.seek(0, os.SEEK_END)
            size_bytes = f.tell()
        if size_bytes <= 0 or size_bytes % SECTOR_SIZE != 0:
            raise ValueError(f"Unexpected disk size: {size_bytes}")

        total_sectors = size_bytes // SECTOR_SIZE
        layout = compute_fat_layout(total_sectors)

        zero_bytes = args.zero_mib * 1024 * 1024
        f.seek(0)
        chunk = b"\x00" * (1024 * 1024)
        remaining = zero_bytes
        while remaining > 0:
            part = chunk if remaining >= len(chunk) else b"\x00" * remaining
            f.write(part)
            remaining -= len(part)

        boot = build_boot_sector(total_sectors, label, layout)
        fsinfo = build_fsinfo(layout["cluster_count"])
        fat0 = build_fat_sector()

        write_all(f, 0, boot)
        write_all(f, SECTOR_SIZE, fsinfo)
        write_all(f, 6 * SECTOR_SIZE, boot)
        write_all(f, 7 * SECTOR_SIZE, fsinfo)

        fat_start = layout["reserved"] * SECTOR_SIZE
        fat_span = layout["fat_sectors"] * SECTOR_SIZE
        zero_fat = b"\x00" * fat_span
        write_all(f, fat_start, zero_fat)
        write_all(f, fat_start + fat_span, zero_fat)
        write_all(f, fat_start, fat0)
        write_all(f, fat_start + fat_span, fat0)
        f.flush()
        os.fsync(f.fileno())

    print(
        f"Wrote FAT32 superfloppy to {args.physical_drive} "
        f"(size={size_bytes}, sectors={total_sectors}, spc={layout['spc']}, "
        f"fat_sectors={layout['fat_sectors']}, clusters={layout['cluster_count']})"
    )


if __name__ == "__main__":
    main()
