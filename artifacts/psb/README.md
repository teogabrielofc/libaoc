Artefatos PSB prontos do libaoc.

- libaocdoom.avi + libaocdoom.psb: abre /mnt/doom/launch.sh.
- libaoccore.avi + libaoccore.psb: tenta gerar crash/core dump no parser.

Constantes padrao:

- libc base = 0x2bbb8000
- saved s0 = 0x41414141

Para comando customizado:

make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd

Ultima geracao:

- artefato = libaoccore.psb
- texto = 340 bytes 'A' + marcador GPGP
- fluxo = cold boot com pendrive superfloppy, abrir AVI e habilitar PSB
