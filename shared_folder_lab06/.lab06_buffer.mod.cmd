savedcmd_lab06_buffer.mod := printf '%s\n'   lab06_buffer.o | awk '!x[$$0]++ { print("./"$$0) }' > lab06_buffer.mod
