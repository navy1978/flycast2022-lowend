#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
repo_root=$(CDPATH= cd -- "$script_dir/.." && pwd -P)
profile_bundle="$repo_root/pgo/rk3566-aica-sonic"
jobs=${JOBS:-4}
profile_cc=${CC:-cc}

case "$jobs" in
	''|*[!0-9]*)
		printf '%s\n' "JOBS must be a positive integer." >&2
		exit 2
		;;
esac

if [ "$jobs" -eq 0 ]; then
	printf '%s\n' "JOBS must be a positive integer." >&2
	exit 2
fi

if [ ! -d "$profile_bundle" ]; then
	printf "Missing RK3566 PGO profile bundle: %s\n" "$profile_bundle" >&2
	exit 1
fi

compiler_version=$("$profile_cc" -dumpfullversion -dumpversion 2>/dev/null || true)
case "$compiler_version" in
	9.*)
		;;
	*)
		printf "The bundled profiles require GCC 9.x; %s reports '%s'.\n" \
			"$profile_cc" "${compiler_version:-unknown}" >&2
		exit 1
		;;
esac

if command -v sha256sum >/dev/null 2>&1; then
	(
		cd "$profile_bundle"
		sha256sum -c SHA256SUMS
	)
fi

profile_stage=$(mktemp -d "${TMPDIR:-/tmp}/flycast-rk3566-pgo.XXXXXX")
cleanup()
{
	rm -rf "$profile_stage"
}
trap cleanup EXIT HUP INT TERM

encoded_root=$(printf '%s' "$repo_root" | sed 's,/,#,g')
profile_count=0

for source_profile in "$profile_bundle"/core#hw#aica#*.gcda; do
	if [ ! -f "$source_profile" ]; then
		continue
	fi

	profile_name=${source_profile##*/}
	cp "$source_profile" "$profile_stage/${encoded_root}#${profile_name}"
	profile_count=$((profile_count + 1))
done

if [ "$profile_count" -ne 6 ]; then
	printf "Expected 6 RK3566 AICA profiles, found %s.\n" "$profile_count" >&2
	exit 1
fi

printf "Building RK3566 PGO core with %s bundled AICA profiles.\n" "$profile_count"
printf "Profile compiler: %s %s.\n" "$profile_cc" "$compiler_version"

make -C "$repo_root" \
	platform=RK3566 FORCE_GLES=1 HAVE_OPENMP=1 HAVE_LTCG=0 \
	clean

make -C "$repo_root" \
	platform=RK3566 FORCE_GLES=1 HAVE_OPENMP=1 HAVE_LTCG=0 \
	PGO_USE=1 PGO_PROFILE_DIR="$profile_stage" \
	PGO_FILTER_FILES='core/hw/aica/.*' \
	-j"$jobs"

printf "RK3566 PGO core ready: %s/flycast_libretro.so\n" "$repo_root"
