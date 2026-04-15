# Launcher PSB

`tools/make_psb.py doom-launcher` gera `libaocdoom.avi` e `libaocdoom.psb`
para TVs AOC compatíveis com esse caminho de legenda PSB.

As constantes padrão abaixo são da LC32D1320 testada:

- base da libc: `0x2bbb8000`
- `system`: `0x2bc07240`
- gadget `system(a0)` implícito: `0x2bc02780`
- marcador `saved s0`: `0x41414141`

Comando do Doom:

```sh
chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh
```

## Gerar core dump

```sh
python tools/make_psb.py core-crash
```

Isso gera:

- `artifacts/psb/libaoccore.avi`
- `artifacts/psb/libaoccore.psb`

Para o core cair no pendrive, o ponto operacional importa:

- use pendrive FAT32 superfloppy
- deixe o pendrive conectado antes de ligar a TV
- ligue a TV com o pendrive já presente
- abra `libaoccore.avi` e habilite `libaoccore.psb`
- após travar, procure `core.*` na raiz do pendrive

## Ler core dump

```sh
python tools/core_addresses.py core.plfApFusion71Di.875.11
```

Saída esperada:

```text
libc_base=0x...
sp=0x...
system=0x...
implicit_a0_gadget=0x...
xdrrec_eof_gadget=0x...
command_ptr=0x...
```

## PSB com comando escolhido

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

Ou diretamente:

```sh
python tools/make_psb.py command --command 'echo OK>/etc/core/libaoc.ok' --base-name libaoccmd
```

Se tiver um core de outro boot/firmware, use:

```sh
python tools/make_psb.py command --core core.plfApFusion71Di.875.11 --command 'echo OK>/etc/core/libaoc.ok'
```

Para outro modelo AOC, trate `--core` como obrigatório até confirmar que as
bases e gadgets batem.
