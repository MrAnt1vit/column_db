#!/usr/bin/env bash
set -euo pipefail

QUERY_NUM="$1"
COLUMNAR="$2"
OUTPUT="$3"
LOGS="$4"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${REPO_ROOT}/build/src/columnar_db"

"${BIN}" query "${QUERY_NUM}" "${COLUMNAR}" "${OUTPUT}" 2> "${LOGS}"