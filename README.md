# libaoc

`libaoc` é um mini-SDK C nativo para TVs AOC baseadas em MIPS/uClibc. Ele
empacota o necessário para compilar apps e abrir esses apps pelo caminho de
legenda PSB do Media Center.

O alvo testado e usado como referência é a AOC LC32D1320. Algumas partes do
repo, principalmente endereços PSB, dispositivos de framebuffer/input e
artefatos prontos, são focadas nesse modelo e nessa família de firmware.

O repo é autocontido:

- `include/` e `src/`: SDK C.
- `runtime/`: entrada MIPS e shims para uClibc.
- `examples/doom/`: adaptador DoomGeneric para a TV.
- `third_party/sysroots/mips_tv/`: sysroot extraído da firmware.
- `artifacts/usb/doom/`: payload pronto para o pendrive.
- `artifacts/psb/`: `libaocdoom.*` e `libaoccore.*`.
- `tools/`: build, PSB, USB superfloppy e leitura de core dump.

## Estado atual validado

- Framebuffer nativo validado via `/dev/hidtv2dge`.
- Controle remoto validado via `aoc_input` com `/tmp/hp_dfb_handler` e
  fallback em `/dev/remote`.
- Teclado USB validado em userland via `usbfs`/HID boot keyboard, exposto pelo
  backend `aoc_usb_kbd`.
- USB tethering de celular ainda não está validado neste firmware. A TV enxerga
  USB host e alguns dispositivos, mas até agora não apareceu interface de rede
  utilizável por userland sem suporte adicional de driver/kernel.

## Escopo de compatibilidade

- Geral: estrutura do SDK, build MIPS/uClibc, runtime mínimo e organização de
  payloads para TVs AOC parecidas.
- LC32D1320: PSB pronto, constantes de `system()`, gadgets, caminhos de input,
  framebuffer e Doom já validado na TV real.
- Outros modelos: devem ser tratados como portas novas; use `libaoccore.*` e
  `tools/core_addresses.py` para recuperar endereços antes de confiar nos PSBs.

## Início rápido

No WSL/Linux com toolchain MIPS no PATH:

```sh
make test
make sdk
make doom
make psb
make corepsb
make usb
```

Sem `make`, rode diretamente:

```sh
python -m pytest -q
bash tools/build_libaoc_wsl.sh
bash examples/doom/build_doom_wsl.sh
python tools/make_psb.py doom-launcher
python tools/make_psb.py core-crash
python tools/make_usb_tree.py --out dist/usb --include-psb
```

Copie o conteúdo de `dist/usb` para a raiz do pendrive:

```text
doom/
libaocdoom.avi
libaocdoom.psb
libaoccore.avi
libaoccore.psb
```

Na TV, abra `libaocdoom.avi` no Media Center e habilite a legenda
`libaocdoom.psb`. O payload roda:

```sh
chmod +x /mnt/doom/launch.sh; /mnt/doom/launch.sh
```

## PSB por comando

Para gerar um PSB que roda um comando escolhido:

```sh
make psb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
make cmdpsb CMD='echo OK>/etc/core/libaoc.ok' BASE=libaoccmd
```

Isso gera `artifacts/psb/libaoccmd.avi` e `artifacts/psb/libaoccmd.psb`.

## Core dump

`libaoccore.*` é o par para tentar gerar core dump do parser PSB. Use pendrive
FAT32 superfloppy, conectado antes de ligar a TV na tomada. Depois do
travamento, procure `core.*` na raiz do pendrive.

Para ler os endereços:

```sh
python tools/core_addresses.py core.plfApFusion71Di.875.11
```

## Compilar app próprio

Um app C com `main()` pode ser compilado assim:

```sh
make app APP=examples/hello/hello.c OUT=artifacts/usb/hello
```

## Ajustes do Doom

- `AOC_FB_PAGES=1` pinta uma página de framebuffer por frame e é o caminho
  padrão jogável.
- `AOC_FB_PAGES=all` pinta tudo e serve como fallback seguro se a imagem não
  aparecer.
- `AOC_INPUT_DEBUG=1` reativa logs crus de input para mapear botões.

O SDK também já expõe o backend `aoc_usb_kbd` para apps que precisem de
teclado USB fora do caminho do controle remoto.

Controles confirmados:

- Vol+ -> frente
- Vol- -> trás
- Menu -> atirar
- CH+ -> virar à esquerda
- CH- -> virar à direita
- Input -> usar/abrir

## Docs

- `docs/quickstart.md`: caminho curto até rodar na TV.
- `docs/architecture.md`: arquitetura do SDK e runtime.
- `docs/psb-launcher.md`: detalhes dos PSBs.
- `docs/troubleshooting.md`: falhas comuns.

Versoes em ingles:

- `README.en.md`
- `docs/quickstart.en.md`
- `docs/architecture.en.md`
- `docs/psb-launcher.en.md`
- `docs/troubleshooting.en.md`
