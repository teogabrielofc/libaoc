# Início rápido

Este fluxo é para TVs AOC com firmware parecido, MIPS/uClibc e Media Center
com legenda externa. Os artefatos prontos (`libaocdoom.*` e `libaoccore.*`)
foram validados na AOC LC32D1320.

## Requisitos

- Windows com WSL/Ubuntu, ou Linux.
- `python` com os pacotes de `requirements.txt`.
- Toolchain MIPS no PATH:
  - `mips-linux-gnu-gcc`
  - `mips-linux-gnu-as`
  - `mips-linux-gnu-ar`
  - `mips-linux-gnu-readelf`
- `ffmpeg` apenas se você for trocar o vídeo AVI base.

## Build normal

```sh
python -m pip install -r requirements.txt
make test
make sdk
make doom
make psb
make corepsb
make usb
```

## Layout do pendrive

A raiz do pendrive deve ficar assim:

```text
doom/
  doom
  doom1.wad
  launch.sh
  README.md
libaocdoom.avi
libaocdoom.psb
libaoccore.avi
libaoccore.psb
```

Abra `libaocdoom.avi` no Media Center e habilite a legenda.

## Nota para outros modelos AOC

Se o modelo não for LC32D1320, não assuma que os endereços PSB são iguais.
Primeiro gere/colete um core com `libaoccore.*`, rode `tools/core_addresses.py`
e gere novamente os PSBs com `--core`.

## PSB customizado

Para gerar um PSB que roda um comando escolhido:

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

O par gerado fica em `artifacts/psb/libaoccmd.avi` e
`artifacts/psb/libaoccmd.psb`.

## Se a imagem não aparecer

Edite `doom/launch.sh` e use:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

Isso usa o caminho seguro de pintar todas as páginas do framebuffer.
