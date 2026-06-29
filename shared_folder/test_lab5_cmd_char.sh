#!/bin/sh
set -eu

MODULE_DIR=${MODULE_DIR:-/mnt/host}
MODULE=${MODULE:-lab5_cmd_char}
DEVICE=${DEVICE:-/dev/lab5_cmd_char}

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

expect_state()
{
	expected=$1
	label=$2
	current=$(dd if="$DEVICE" bs=1 count=1 2>/dev/null)
	[ "$current" = "$expected" ] || \
		fail "$label: esperado $expected, obtido '$current'"
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

expect_state 0 "estado inicial"

printf 1 > "$DEVICE"
expect_state 1 "estado apos escrever 1"

printf 0 > "$DEVICE"
expect_state 0 "estado apos escrever 0"

printf on > "$DEVICE"
expect_state 1 "estado apos comando on"

printf toggle > "$DEVICE"
expect_state 0 "estado apos primeiro toggle"

printf toggle > "$DEVICE"
expect_state 1 "estado apos segundo toggle"

echo off > "$DEVICE"
expect_state 0 "estado apos comando off com newline"

printf on > "$DEVICE"
expect_state 1 "estado antes de reset"

printf reset > "$DEVICE"
expect_state 0 "estado apos reset"

if printf x > "$DEVICE" 2>/dev/null; then
	fail "escrita invalida deveria falhar com EINVAL"
fi

echo "resultado=ok"
echo "module=$MODULE"
echo "device=$DEVICE"
echo "major=$major"
