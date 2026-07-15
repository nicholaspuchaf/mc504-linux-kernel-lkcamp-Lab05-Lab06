#!/bin/sh
set -eu

MODULE_DIR=${MODULE_DIR:-/mnt/host}
MODULE=${MODULE:-lab06_buffer}
DEVICE=${DEVICE:-/dev/lab06_buffer}
TEST_BIN=${TEST_BIN:-./test_lab06_buffer.static}

cleanup()
{
	rm -f "$DEVICE"
	if lsmod | awk '{print $1}' | grep -qx "$MODULE"; then
		rmmod "$MODULE"
	fi
}

fail()
{
	echo "erro: $*" >&2
	exit 1
}

trap cleanup EXIT

cd "$MODULE_DIR"

if [ ! -r /proc/devices ]; then
	mount -t proc proc /proc
fi

if [ ! -d /sys/module ]; then
	mount -t sysfs sysfs /sys
fi

if lsmod | awk '{print $1}' | grep -qx "$MODULE"; then
	rmmod "$MODULE"
fi

insmod "./$MODULE.ko"

major=$(awk -v name="$MODULE" '$2 == name { print $1 }' /proc/devices)
[ -n "$major" ] || fail "major do modulo $MODULE nao apareceu em /proc/devices"

rm -f "$DEVICE"
mknod "$DEVICE" c "$major" 0

[ -x "$TEST_BIN" ] || fail "binario de teste nao encontrado ou sem permissao de execucao: $TEST_BIN"
"$TEST_BIN"

echo "module=$MODULE"
echo "device=$DEVICE"
echo "major=$major"
