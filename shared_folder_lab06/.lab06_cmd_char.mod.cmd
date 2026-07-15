savedcmd_lab06_cmd_char.mod := printf '%s\n'   lab06_cmd_char.o | awk '!x[$$0]++ { print("./"$$0) }' > lab06_cmd_char.mod
