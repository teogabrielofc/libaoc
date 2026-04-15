# Solução de problemas

As soluções abaixo valem principalmente para a LC32D1320 e para TVs AOC com
firmware parecido. Em outro modelo, primeiro confirme caminhos de dispositivo,
endereços PSB e comportamento do Media Center.

## Doom não aparece

Use o modo seguro de framebuffer:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

Depois rode o launcher PSB de novo.

## Doom aparece mas fica lento

Use o padrão jogável:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-1}"
```

Deixe `AOC_INPUT_DEBUG=0`; log cru de input faz escrita demais no filesystem.

## Botões não funcionam

Ative debug de input:

```sh
export AOC_INPUT_DEBUG="${AOC_INPUT_DEBUG:-1}"
```

Depois veja `doom/launch_doom_usb.log` e `/etc/core/doom.log`.

## PSB não faz nada

Confirme que a raiz do pendrive contem:

- `libaocdoom.avi`
- `libaocdoom.psb`

Também confirme que a legenda está habilitada no Media Center.

## Core dump não aparece

O firmware monta `/etc/core` durante o boot. Se o pendrive entrou depois da TV
ligada, o crash pode acontecer e mesmo assim nenhum core cair no USB.

Use este fluxo:

1. formate o pendrive como FAT32 superfloppy
2. coloque `libaoccore.avi` e `libaoccore.psb` na raiz
3. deixe o pendrive conectado
4. tire e coloque a TV da tomada
5. abra `libaoccore.avi`
6. habilite a legenda
7. após o travamento, confira `core.*` no PC

## Endereços mudaram

Rode:

```sh
python tools/core_addresses.py core.plfApFusion71Di.875.11
```

Depois gere o PSB de novo com:

```sh
python tools/make_psb.py doom-launcher --core core.plfApFusion71Di.875.11
```

Se for outra TV AOC, gere os PSBs sempre a partir do core desse próprio modelo.
