#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
	echo "Uso: $0 baseline|audio-thread|palette-fog|sh4-fpscr ROM" >&2
	exit 2
fi

REVIEW_DIR=/storage/retrorun-test/flycast2022-manual-followup
FRONTEND=/storage/retrorun-test/retrorun
SAVE_DIR="$REVIEW_DIR/sonic-save"
CANDIDATE=$1
ROM=$2

case "$CANDIDATE" in
	baseline)
		CORE="$REVIEW_DIR/flycast2022-baseline-subimage_libretro.so"
		CONFIG="$REVIEW_DIR/baseline.cfg"
		;;
	audio-thread)
		CORE="$REVIEW_DIR/flycast2022-baseline-subimage_libretro.so"
		CONFIG="$REVIEW_DIR/audio-thread.cfg"
		;;
	palette-fog)
		CORE="$REVIEW_DIR/flycast2022-baseline-subimage_libretro.so"
		CONFIG="$REVIEW_DIR/palette-fog.cfg"
		;;
	sh4-fpscr)
		CORE="$REVIEW_DIR/flycast2022-sh4-fpscr_libretro.so"
		CONFIG="$REVIEW_DIR/baseline.cfg"
		;;
	*)
		echo "Candidata sconosciuta: $CANDIDATE" >&2
		exit 2
		;;
esac

for file in "$FRONTEND" "$CORE" "$CONFIG" "$ROM"; do
	if [ ! -f "$file" ]; then
		echo "File non trovato: $file" >&2
		exit 3
	fi
done

if pgrep retrorun >/dev/null 2>&1; then
	echo "Un processo RetroRun è già attivo." >&2
	pgrep -a retrorun >&2 || true
	exit 5
fi

ES_WAS_ACTIVE=0
restore_es()
{
	if [ "$ES_WAS_ACTIVE" -eq 1 ]; then
		systemctl start emustation 2>/dev/null || true
	fi
}
trap restore_es EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if systemctl is-active --quiet emustation; then
	ES_WAS_ACTIVE=1
fi

# This full restart is required on the test RG351V. A shorter hand-off has
# intermittently left the panel black even when Flycast was presenting frames.
systemctl stop emustation
systemctl start emustation
sleep 6
systemctl stop emustation
sleep 3

echo "Candidata: $CANDIDATE"
echo "Core: $CORE"
echo "Config: $CONFIG"
echo "ROM: $ROM"

"$FRONTEND" \
	-c "$CONFIG" \
	-d /storage/roms/bios \
	-s "$SAVE_DIR" \
	"$CORE" "$ROM"
