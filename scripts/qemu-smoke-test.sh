#!/usr/bin/env bash
# Boots the ISO in a headless QEMU and checks the serial output.
# Expect the startup greeting followed by the post-boot timer message.
set -euo pipefail

ISO="${1:-build/construction-os.iso}"
LOG="${2:-build/serial.log}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-20}"
BOOT_MARKER="Hello from construction-os!"
TIMER_MARKER="PIT: 100 ticks after boot"
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
  if awk -v boot="$BOOT_MARKER" -v timer="$TIMER_MARKER" '
    $0 == boot { booted = 1 }
    booted && $0 == timer { received_ticks = 1 }
    END { exit !received_ticks }
  ' "$LOG" 2>/dev/null; then
    echo "PASS: kernel booted and received 100 more IRQ0 ticks. Serial output:"
    cat "$LOG"
    exit 0
  fi
  if ! kill -0 "$QEMU_PID" 2>/dev/null; then
    break # QEMU exited early (e.g. triple fault with -no-reboot)
  fi
done

echo "FAIL: expected boot and timer messages in order (timeout or crash). Serial output:"
cat "$LOG" 2>/dev/null || echo "(empty)"
exit 1
