#!/bin/sh
set -eu

if [ "$#" -ne 5 ]; then
    echo "usage: $0 CORE ROM JSON LOG SECONDS" >&2
    exit 2
fi

FRONTEND=${FRONTEND:-/storage/retrorun-test/retrorun}
CONFIG=${CONFIG:-/storage/retrorun-test/flycast2022-profiler/flycast2022-profiler-retrorun.cfg}
SAVE_DIR=${SAVE_DIR:-/storage/roms/retrorun-test-saves}
CORE=$1
ROM=$2
JSON=$3
LOG=$4
BENCHMARK_SECONDS=$5
BENCHMARK_WARMUP_SECONDS=${BENCHMARK_WARMUP_SECONDS:-8}
BENCHMARK_CORE_FRAMES=${BENCHMARK_CORE_FRAMES:-0}
BENCHMARK_CONFIRM_INPUT=${BENCHMARK_CONFIRM_INPUT:-true}
BENCHMARK_CONFIRM_INPUT_DELAY=${BENCHMARK_CONFIRM_INPUT_DELAY:-4}
BENCHMARK_FRAME_ARGS=

case "$BENCHMARK_CORE_FRAMES" in
    0) ;;
    *[!0-9]*|'')
        echo "BENCHMARK_CORE_FRAMES must be a non-negative integer." >&2
        exit 2
        ;;
    *) BENCHMARK_FRAME_ARGS="--benchmark-frames $BENCHMARK_CORE_FRAMES" ;;
esac

if pgrep retrorun >/dev/null 2>&1; then
    echo "A RetroRun process is already active." >&2
    pgrep -a retrorun >&2 || true
    exit 5
fi

CPU=/sys/devices/system/cpu/cpufreq/policy0
DMC=/sys/class/devfreq/dmc
GPU=/sys/class/devfreq/ff400000.gpu
ES_WAS_ACTIVE=0

restore_system()
{
    echo 912000 > "$CPU/scaling_min_freq" 2>/dev/null || true
    echo 1296000 > "$CPU/scaling_max_freq" 2>/dev/null || true
    echo ondemand > "$CPU/scaling_governor" 2>/dev/null || true
    echo 528000000 > "$DMC/min_freq" 2>/dev/null || true
    echo 786000000 > "$DMC/max_freq" 2>/dev/null || true
    echo dmc_ondemand > "$DMC/governor" 2>/dev/null || true
    echo 480000000 > "$GPU/min_freq" 2>/dev/null || true
    echo 520000000 > "$GPU/max_freq" 2>/dev/null || true
    echo dmc_ondemand > "$GPU/governor" 2>/dev/null || true
    if [ "$ES_WAS_ACTIVE" -eq 1 ]; then
        systemctl start emustation 2>/dev/null || true
    fi
}
trap restore_system EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if systemctl is-active --quiet emustation; then
    ES_WAS_ACTIVE=1
fi
# Force a complete KMS/DRM modeset before every run.  On this RG351V a
# restart followed by only three seconds can stop EmulationStation before its
# display initialization has settled, leaving the following process black.
systemctl stop emustation
systemctl start emustation
sleep 6
systemctl stop emustation
sleep 3

echo performance > "$CPU/scaling_governor"
echo 1296000 > "$CPU/scaling_max_freq"
echo 1296000 > "$CPU/scaling_min_freq"
echo userspace > "$DMC/governor"
echo 786000000 > "$DMC/max_freq"
echo 786000000 > "$DMC/min_freq"
echo userspace > "$GPU/governor"
echo 520000000 > "$GPU/max_freq"
echo 520000000 > "$GPU/min_freq"

FLYCAST_LOWEND_PROFILE_INTERVAL=60 "$FRONTEND" \
    -c "$CONFIG" \
    -d /storage/roms/bios \
    -s "$SAVE_DIR" \
    --benchmark "$BENCHMARK_SECONDS" \
    --benchmark-warmup "$BENCHMARK_WARMUP_SECONDS" \
    $BENCHMARK_FRAME_ARGS \
    --benchmark-json "$JSON" \
    --benchmark-set confirm_input="$BENCHMARK_CONFIRM_INPUT" \
    --benchmark-set confirm_input_delay="$BENCHMARK_CONFIRM_INPUT_DELAY" \
    "$CORE" "$ROM" > "$LOG" 2>&1
