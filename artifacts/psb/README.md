PSB launcher for /mnt/doom/launch.sh using the working implicit-a0 path.

- `libc base = 0x2bbb8000`
- `system = 0x2bc07240`
- `libc + 0x4a780 = 0x2bc02780`
- `saved s0 = 0x41414141`
- `command = chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh #`

Expected behavior:
- PSB executes shell command through system(a0)
- command chmods /mnt/doom/launch.sh
- command runs /mnt/doom/launch.sh
