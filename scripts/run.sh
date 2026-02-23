#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
MODE="flat"
MODEL="head"
INTENSITY=""
YAW=""

usage() {
	cat <<EOF
Usage: $(basename "$0") [options] [mode] [model]

Options:
	--mode <mode>           wireframe|flat|smooth|texture|uv|normal|depth|shadowmap|ssao|complete
  --model <model>         head|african_head|diablo3_pose
	--intensity <float>     Main light intensity (example: 2.0)
	--yaw <degrees>         Rotate model around Y axis (example: 45)

Convenience mode flags (equivalent to --mode <mode>):
	--wireframe | --flat | --smooth | --texture | --uv | --normal | --depth | --shadowmap | --ssao | --complete

Examples:
  ./scripts/run.sh --flat --model head
  ./scripts/run.sh --mode ssao --model diablo3_pose
  ./scripts/run.sh normal diablo3_pose
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--mode)
			MODE="$2"; shift 2 ;;
		--model)
			MODEL="$2"; shift 2 ;;
		--intensity)
			INTENSITY="$2"; shift 2 ;;
		--yaw)
			YAW="$2"; shift 2 ;;
		--wireframe|--flat|--smooth|--texture|--uv|--normal|--depth|--shadowmap|--ssao|--complete)
			MODE="${1#--}"; shift 1 ;;
		-h|--help)
			usage; exit 0 ;;
		*)
			# positional compatibility: [mode] [model]
			if [[ -z "${MODE_FROM_POSITIONAL:-}" ]]; then
				MODE="$1"; MODE_FROM_POSITIONAL=1; shift 1
			elif [[ -z "${MODEL_FROM_POSITIONAL:-}" ]]; then
				MODEL="$1"; MODEL_FROM_POSITIONAL=1; shift 1
			else
				echo "Unknown argument: $1" >&2
				usage
				exit 2
			fi
			;;
	esac
done

EXTRA_ARGS=()
if [[ -n "$INTENSITY" ]]; then
	EXTRA_ARGS+=("--intensity" "$INTENSITY")
fi
if [[ -n "$YAW" ]]; then
	EXTRA_ARGS+=("--yaw" "$YAW")
fi

"${BUILD_DIR}/minirenderer" "$MODE" "$MODEL" "${EXTRA_ARGS[@]}"
