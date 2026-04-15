# Arquitetura

`libaoc` é uma biblioteca C estática para apps nativos em TVs AOC
MIPS/uClibc. A ideia é deixar os detalhes chatos da TV fora dos apps.

A arquitetura é geral, mas o backend atual foi levantado e validado na
LC32D1320. Portar para outra AOC provavelmente exige confirmar dispositivos,
endereços PSB e layout de framebuffer/input.

## Módulos do SDK

- `aoc_fb.c`: abre `/dev/hidtv2dge`, lê geometria, mapeia framebuffer e mostra
  frames XRGB8888.
- `aoc_input.c`: usa `/tmp/hp_dfb_handler`, decodifica pacotes do controle e
  tenta fallback em `/dev/remote`.
- `aoc_log.c`: escreve logs pequenos no filesystem da TV e pode chamar
  `fsync`.
- `aoc_runtime.c`: wrappers pequenos para syscall MIPS, tempo e sleep.

## Runtime

Binários da TV usam:

- `runtime/start.S`: entrada do processo MIPS/uClibc.
- `runtime/appinit.c`: `_init` e `_fini` vazios.
- `runtime/uclibc_compat.c`: shims para símbolos estilo glibc que o compilador
  pode emitir.

## Exemplo Doom

O adaptador do DoomGeneric apenas traduz hooks para chamadas `libaoc`:

- `DG_Init` abre framebuffer e input.
- `DG_DrawFrame` chama `aoc_fb_present_xrgb8888_scaled`.
- `DG_GetKey` consulta eventos do `libaoc` e converte para teclas do Doom.

O modo padrão pinta uma página de framebuffer por frame. Use
`AOC_FB_PAGES=all` se outra unidade ou firmware mostrar a página errada.

## PSB

`tools/make_psb.py` gera três famílias:

- `doom-launcher`: `libaocdoom.*`, abre `/mnt/doom/launch.sh`.
- `core-crash`: `libaoccore.*`, tenta gerar core dump do parser.
- `command`: nome escolhido por `--base-name`, roda um comando shell escolhido.

O PSB customizado usa o mesmo caminho `system(a0)` já validado no aparelho.
As constantes padrão desse caminho são da LC32D1320; para outro modelo, gere
um core e regenere o PSB com `--core`.
