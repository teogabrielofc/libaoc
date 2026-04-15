# Inicio rapido

## Requisitos

- Windows com WSL/Ubuntu, ou Linux.
- `python` com os pacotes de `requirements.txt`.
- Toolchain MIPS no PATH:
  - `mips-linux-gnu-gcc`
  - `mips-linux-gnu-as`
  - `mips-linux-gnu-ar`
  - `mips-linux-gnu-readelf`
- `ffmpeg` apenas se voce for trocar o video AVI base.

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

## PSB customizado

Para gerar um PSB que roda um comando escolhido:

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

O par gerado fica em `artifacts/psb/libaoccmd.avi` e
`artifacts/psb/libaoccmd.psb`.

## Se a imagem nao aparecer

Edite `doom/launch.sh` e use:

```sh
export AOC_FB_PAGES="${AOC_FB_PAGES:-all}"
```

Isso usa o caminho seguro de pintar todas as paginas do framebuffer.
