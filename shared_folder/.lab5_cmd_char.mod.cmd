savedcmd_lab5_cmd_char.mod := printf '%s\n'   lab5_cmd_char.o | awk '!x[$$0]++ { print("./"$$0) }' > lab5_cmd_char.mod
