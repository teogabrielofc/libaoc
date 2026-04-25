# Kernel Tether Modules

Esta pasta concentra a rota de kernel para USB tethering na AOC LC32D1320 e
firmwares parecidos.

## Por que existe

O SDK/userland já conseguiu provar:

- `telnetd` existe na TV
- host USB existe
- USB networking parcial existe no kernel
- mas tethering de celular ainda não sobe interface útil em userland

O motivo mais forte, confirmado pelo `mtd1.kernel.config`, é que o kernel
atual não tem:

- `CONFIG_USB_NET_RNDIS_HOST`
- `CONFIG_USB_NET_CDCETHER`

Ao mesmo tempo, ele já tem:

- `CONFIG_MODULES=y`
- `CONFIG_MODULE_UNLOAD=y`
- `CONFIG_USB_USBNET=y`
- `CONFIG_MII=y`

Então a rota certa não é “mais app em userland”; é preparar módulos
compatíveis com esse kernel:

- `rndis_host`
- `cdc_ether`

## Alvo exato

Os módulos nativos da TV mostram este `vermagic`:

```text
2.6.18_pro500.default preempt mod_unload MIPS32_R2 32BIT gcc-4.2
```

Isso importa porque:

- `EXTRAVERSION` precisa bater em `_pro500`
- `CONFIG_LOCALVERSION` precisa ficar `.default`
- o `vermagic` também embute `gcc-4.2`, então o build moderno precisa
  acomodar isso

## Conteúdo

- `config/lc32d1320-2.6.18_pro500.default.config`
  - cópia da config de kernel da TV
- `vendor/linux-2.6.18/drivers/usb/net/`
  - cópias oficiais do Linux 2.6.18 para `rndis_host.c`, `cdc_ether.c`,
    `usbnet.h`, `Kconfig` e `Makefile`
- `scripts/prepare_kernel_tree_wsl.sh`
  - baixa e prepara a árvore base de kernel no WSL
- `scripts/build_tether_modules_wsl.sh`
  - tenta gerar `rndis_host.ko` e `cdc_ether.ko`

## Observação importante sobre a árvore base

A árvore oficial do Linux 2.6.18 não conhece todos os patches de vendor da TV.
Na prática, isso significa que um `oldconfig` puro pode derrubar o menu USB
que existe no firmware real, mesmo com a config extraída da TV.

Por isso o staging daqui faz duas coisas ao mesmo tempo:

- preserva `EXTRAVERSION`, `CONFIG_LOCALVERSION` e `vermagic`
- força `rndis_host` e `cdc_ether` como módulos no build de staging com
  `AOC_TETHER_FORCE_MODULES=1`
- força a família de processador do `vermagic` para `MIPS32_R2`, para ficar
  alinhada com os módulos nativos observados na TV

Isso não substitui a validação na TV. Só remove o falso bloqueio causado pela
árvore vanilla.

## Estado

Isto é uma base de trabalho séria, mas ainda não deve ser tratado como
“módulo validado na TV”.

O próximo sucesso real desta pasta é:

1. compilar os `.ko`
2. verificar `vermagic`
3. carregar na TV
4. confirmar se o tethering cria interface com IP
