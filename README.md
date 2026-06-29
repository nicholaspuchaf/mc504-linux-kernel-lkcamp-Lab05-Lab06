# Lab 5: syscalls e device drivers no Linux

Este repositorio contem uma arvore local do kernel Linux usada para dois exercicios praticos:

- adicionar chamadas de sistema proprias ao kernel;
- implementar e testar modulos de character device carregaveis.

O trabalho foi validado em QEMU usando o kernel compilado em `linux/arch/x86/boot/bzImage`, o disco `my_disk.raw` e pastas compartilhadas via 9p para levar os testes do host para a VM.

## Estrutura do repositorio

- `linux/`: arvore do kernel modificada e compilada.
- `my_disk.raw`: disco usado para inicializar a VM.
- `shared_folder_lab05/`: programa de teste das syscalls.
- `shared_folder_lab06/`: fontes, scripts e artefatos compilados dos character devices.
- `../PLANO_SYSCALLS.md`: relatorio detalhado da etapa de syscalls.
- `../PLANO_DEVICE_DRIVERS.md`: relatorio detalhado da etapa de device drivers.

## Parte 1: chamadas de sistema

A primeira etapa adicionou tres syscalls novas ao kernel x86-64:

| Numero | Nome | Funcao |
| --- | --- | --- |
| `472` | `lab5_get_value` | copia para userspace o valor inteiro armazenado no kernel |
| `473` | `lab5_set_value` | atualiza o valor inteiro armazenado no kernel |
| `474` | `lab5_stats` | retorna estatisticas sobre o uso das syscalls |

O estado mantido pelo kernel fica em `linux/kernel/lab5_syscalls.c`. A implementacao guarda:

- valor atual (`lab5_kernel_value`);
- numero de chamadas a `lab5_get_value`;
- numero de chamadas a `lab5_set_value`;
- ultimo valor escrito;
- PID do ultimo processo que executou `lab5_set_value`.

O acesso a esse estado e protegido por `spinlock`, porque `lab5_stats` precisa montar um snapshot consistente de valor, contadores e metadados.

### Arquivos modificados para registrar as syscalls

- `linux/kernel/lab5_syscalls.c`: implementacao das syscalls com `SYSCALL_DEFINE1`.
- `linux/kernel/Makefile`: inclui `lab5_syscalls.o` no build do kernel.
- `linux/arch/x86/entry/syscalls/syscall_64.tbl`: registra os numeros `472`, `473` e `474` para x86-64.
- `linux/include/linux/syscalls.h`: declara os prototipos das novas syscalls.
- `linux/include/uapi/asm-generic/unistd.h`: expoe os numeros UAPI e atualiza `__NR_syscalls` para `475`.
- `linux/include/uapi/linux/lab5_syscalls.h`: define `struct lab5_stats`.
- `linux/kernel/sys_ni.c`: adiciona fallback com `COND_SYSCALL`.

### Teste de userspace

O teste fica em `shared_folder_lab05/test_lab5_syscalls.c` e chama as syscalls diretamente com `syscall()`, ja que a libc do sistema convidado nao conhece essas chamadas novas.

O binario estatico `shared_folder_lab05/test_lab5_syscalls` valida:

- leitura do valor inicial;
- escrita de um novo valor;
- nova leitura do valor;
- estatisticas retornadas por `lab5_stats`;
- incremento correto dos contadores;
- `last_written_value` e `last_set_pid`.

Exemplos de valores testados na VM:

```sh
/mnt/host/test_lab5_syscalls 123
/mnt/host/test_lab5_syscalls -7
```

As duas execucoes terminaram com `resultado=ok`, incluindo o caso negativo. Esse caso foi importante porque mostrou que retornar o valor diretamente pela syscall seria incorreto: retornos negativos sao interpretados como `-errno`. A ABI final usa `copy_to_user()` em `lab5_get_value`.

## Parte 2: character devices

A segunda etapa implementou dois modulos out-of-tree em `shared_folder_lab06/`.

### `lab5_char`

`shared_folder_lab06/lab5_char.c` implementa um character device simples chamado `lab5_char`.

Comportamento:

- leitura retorna um byte: `0` ou `1`;
- escrita de `0` desliga o estado interno;
- escrita de `1` liga o estado interno;
- escrita de qualquer outro primeiro caractere retorna `-EINVAL`.

O modulo usa:

- `alloc_chrdev_region()` para obter major dinamico;
- `cdev_init()` e `cdev_add()` para registrar o character device;
- `copy_to_user()` e `copy_from_user()` para atravessar a fronteira kernel/userspace;
- `mutex` para proteger o estado interno.

### `lab5_cmd_char`

`shared_folder_lab06/lab5_cmd_char.c` e uma versao com comandos textuais, registrada como `lab5_cmd_char`.

Comandos aceitos:

- `1` ou `on`: liga;
- `0` ou `off`: desliga;
- `toggle`: inverte o estado;
- `reset`: volta para desligado.

A leitura continua retornando apenas `0` ou `1`. O driver usa `strim()` para aceitar comandos com quebra de linha, como `echo off > /dev/lab5_cmd_char`.

### Build dos modulos

O `Makefile` em `shared_folder_lab06/` declara:

```make
obj-m += lab5_char.o
obj-m += lab5_cmd_char.o
```

Compilacao a partir da raiz deste repositorio:

```sh
make -C linux M=$PWD/shared_folder_lab06 modules
```

O build gera, entre outros artefatos:

- `shared_folder_lab06/lab5_char.ko`;
- `shared_folder_lab06/lab5_cmd_char.ko`;
- objetos `.o`, arquivos `.mod*`, `Module.symvers` e `modules.order`.

## Executando na VM

Comando base para iniciar a VM com a pasta de syscalls compartilhada:

```sh
qemu-system-x86_64 \
  -drive file=my_disk.raw,format=raw,index=0,media=disk \
  -m 2G \
  -nographic \
  -kernel linux/arch/x86/boot/bzImage \
  -append "root=/dev/sda rw console=ttyS0 loglevel=6 init=/bin/sh tsc=unstable" \
  -fsdev local,id=fs1,path=shared_folder_lab05,security_model=none \
  -device virtio-9p-pci,fsdev=fs1,mount_tag=shared_folder
```

Dentro da VM:

```sh
mkdir -p /mnt/host
mount -t 9p -o trans=virtio shared_folder /mnt/host
/mnt/host/test_lab5_syscalls 123
```

Para testar os character devices, iniciar a VM trocando o `path` para `shared_folder_lab06`:

```sh
qemu-system-x86_64 \
  -drive file=my_disk.raw,format=raw,index=0,media=disk \
  -m 2G \
  -nographic \
  -kernel linux/arch/x86/boot/bzImage \
  -append "root=/dev/sda rw console=ttyS0 loglevel=6 init=/bin/sh tsc=unstable" \
  -fsdev local,id=fs1,path=shared_folder_lab06,security_model=none \
  -device virtio-9p-pci,fsdev=fs1,mount_tag=shared_folder
```

Dentro da VM:

```sh
mkdir -p /mnt/host
mount -t 9p -o trans=virtio shared_folder /mnt/host
sh /mnt/host/test_lab5_char.sh
sh /mnt/host/test_lab5_cmd_char.sh
```

Os scripts:

- carregam o modulo com `insmod`;
- descobrem o major dinamico em `/proc/devices`;
- criam o no em `/dev` com `mknod`;
- fazem leituras e escritas de validacao;
- verificam erro em escrita invalida;
- removem o no e descarregam o modulo ao sair.

Saida esperada:

```text
resultado=ok
device=/dev/lab5_char
major=<numero dinamico>
resultado=ok
module=lab5_cmd_char
device=/dev/lab5_cmd_char
major=<numero dinamico>
```

Na execucao registrada, ambos os testes passaram e o major dinamico observado foi `247`.

## Principais decisoes e problemas resolvidos

- Os numeros do tutorial original nao foram reutilizados: esta arvore ja tinha syscalls ate `471`, entao as novas chamadas foram registradas em `472`, `473` e `474`.
- `lab5_get_value` foi desenhada para escrever em ponteiro de userspace, nao para retornar o valor diretamente, preservando valores negativos validos.
- `lab5_stats` copia um snapshot local depois de soltar o `spinlock`, evitando acessar memoria de usuario enquanto o lock esta segurado.
- Os drivers usam major dinamico, entao os testes consultam `/proc/devices` antes de criar os nos em `/dev`.
- Os scripts de teste montam `/proc` e `/sys` quando necessario, porque o boot com `init=/bin/sh` pode iniciar sem esses pseudo-filesystems prontos.

## Status final

- Kernel recompilado com as tres syscalls novas.
- Programa de teste das syscalls compilado estaticamente e validado na VM.
- Dois character devices implementados como modulos externos.
- Modulos `lab5_char.ko` e `lab5_cmd_char.ko` compilados contra a mesma arvore do kernel usada no QEMU.
- Testes automatizados dos drivers executados com sucesso dentro da VM.
