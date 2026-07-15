savedcmd_lab06_char.mod := printf '%s\n'   lab06_char.o | awk '!x[$$0]++ { print("./"$$0) }' > lab06_char.mod
