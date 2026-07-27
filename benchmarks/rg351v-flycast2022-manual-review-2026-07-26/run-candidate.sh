#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
	echo "Uso: $0 CANDIDATA ROM" >&2
	echo "Candidate: baseline div1 shadow subimage pal4" >&2
	exit 2
fi

REVIEW_DIR=/storage/retrorun-test/flycast2022-candidates-review
FRONTEND=/storage/retrorun-test/retrorun
CONFIG="$REVIEW_DIR/retrorun.cfg"
SAVE_DIR=/storage/roms/retrorun-test-saves
CANDIDATE=$1
ROM=$2

case "$CANDIDATE" in
	baseline) CORE="$REVIEW_DIR/flycast2022-baseline_libretro.so" ;;
	div1) CORE="$REVIEW_DIR/flycast2022-div1_libretro.so" ;;
	shadow) CORE="$REVIEW_DIR/flycast2022-texture-shadow_libretro.so" ;;
	subimage) CORE="$REVIEW_DIR/flycast2022-texture-subimage_libretro.so" ;;
	pal4) CORE="$REVIEW_DIR/flycast2022-pal4-pair_libretro.so" ;;
	*)
		echo "Candidata sconosciuta: $CANDIDATE" >&2
		exit 2
		;;
esac

if [ ! -f "$CORE" ]; then
	echo "Core non trovato: $CORE" >&2
	exit 3
fi
if [ ! -f "$ROM" ]; then
	echo "ROM non trovata: $ROM" >&2
	exit 4
fi
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

# Complete modeset required by this RG351V before every Flycast test.
systemctl stop emustation
systemctl start emustation
sleep 6
systemctl stop emustation
sleep 3

echo "Candidata: $CANDIDATE"
echo "Core: $CORE"
echo "ROM: $ROM"
echo "Config: $CONFIG"

"$FRONTEND" \
	-c "$CONFIG" \
	-d /storage/roms/bios \
	-s "$SAVE_DIR" \
	"$CORE" "$ROM"
