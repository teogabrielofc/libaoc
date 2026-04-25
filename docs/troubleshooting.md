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

## Teclado USB não funciona

O caminho de teclado USB do SDK não passa pelo mesmo backend do controle
remoto. Ele usa `usbfs`/HID direto em userland.

Se um app seu depende de teclado:

- use o backend `aoc_usb_kbd`, não apenas `aoc_input`
- não assuma que `/dev/keyboard`, `/dev/input/event*` ou `/tmp/hp_dfb_handler`
  vão servir para o teclado nesse firmware
- confirme se o dispositivo realmente enumera em `/proc/bus/usb/devices`

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

## USB tethering não sobe rede

O estado atual do firmware testado é:

- `telnetd` existe
- host USB existe
- mas o tethering de celular ainda não expôs uma interface de rede utilizável
  em userland

Então, se o celular conectar e nada como `eth0`/`usb0`/`rndis0` ganhar IP, o
problema mais provável não é o `libaoc` em si, e sim suporte ausente ou
incompleto de driver/kernel para esse modo de rede.
