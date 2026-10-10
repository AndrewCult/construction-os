#!/usr/bin/env bash
# Boots the ISO in a headless QEMU and checks the serial output.
# Passes when the kernel prints its startup message; fails on a CPU exception,
# a boot error, or if nothing happens within the timeout (hang or triple fault).
set -euo pipefail

ISO="${1:-build/construction-os.iso}"
LOG="${2:-build/serial.log}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-20}"
SUCCESS_MARKER="Hello from construction-os!"
FAILURE_PATTERN="CPU EXCEPTION|ERROR:"

if [[ ! -f "$ISO" ]]; then
  echo "ISO not found: $ISO (run 'make iso' first)" >&2
  exit 2
fi

rm -f "$LOG"

# -display none: no window; -serial file: capture COM1 output in the log.
# -no-reboot: a triple fault stops QEMU instead of rebooting in a loop.
qemu-system-i386 \
  -cdrom "$ISO" \
  -m 128M \
  -display none \
  -serial "file:$LOG" \
  -no-reboot \
  -no-shutdown &
QEMU_PID=$!
trap 'kill "$QEMU_PID" 2>/dev/null || true' EXIT

for ((elapsed = 0; elapsed < TIMEOUT_SECONDS; elapsed++)); do
  sleep 1
  if grep -qE "$FAILURE_PATTERN" "$LOG" 2>/dev/null; then
    echo "FAIL: the kernel reported an error. Serial output:"
    cat "$LOG"
    exit 1
  fi
  if grep -qF "$SUCCESS_MARKER" "$LOG" 2>/dev/null; then
    echo "PASS: kernel booted in ${elapsed}s. Serial output:"
    cat "$LOG"
    exit 0
  fi
  if ! kill -0 "$QEMU_PID" 2>/dev/null; then
    break # QEMU exited early (e.g. triple fault with -no-reboot)
  fi
done

echo "FAIL: startup message not found (timeout or crash). Serial output:"
cat "$LOG" 2>/dev/null || echo "(empty)"
exit 1
