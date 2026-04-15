# Arquitetura

`libaoc` e uma biblioteca C estatica. A ideia e deixar os detalhes chatos da
TV fora dos apps.

## Modulos do SDK

- `aoc_fb.c`: abre `/dev/hidtv2dge`, le geometria, mapeia framebuffer e mostra
  frames XRGB8888.
- `aoc_input.c`: usa `/tmp/hp_dfb_handler`, decodifica pacotes do controle e
  tenta fallback em `/dev/remote`.
- `aoc_log.c`: escreve logs pequenos no filesystem da TV e pode chamar
  `fsync`.
- `aoc_runtime.c`: wrappers pequenos para syscall MIPS, tempo e sleep.

## Runtime

Binarios da TV usam:

- `runtime/start.S`: entrada do processo MIPS/uClibc.
- `runtime/appinit.c`: `_init` e `_fini` vazios.
- `runtime/uclibc_compat.c`: shims para simbolos estilo glibc que o compilador
  pode emitir.

## Exemplo Doom

O adaptador do DoomGeneric apenas traduz hooks para chamadas `libaoc`:

- `DG_Init` abre framebuffer e input.
- `DG_DrawFrame` chama `aoc_fb_present_xrgb8888_scaled`.
- `DG_GetKey` consulta eventos do `libaoc` e converte para teclas do Doom.

O modo padrao pinta uma pagina de framebuffer por frame. Use
`AOC_FB_PAGES=all` se outra unidade ou firmware mostrar a pagina errada.

## PSB

`tools/make_psb.py` gera tres familias:

- `doom-launcher`: `libaocdoom.*`, abre `/mnt/doom/launch.sh`.
- `core-crash`: `libaoccore.*`, tenta gerar core dump do parser.
- `command`: nome escolhido por `--base-name`, roda um comando shell escolhido.

O PSB customizado usa o mesmo caminho `system(a0)` ja validado no aparelho.
