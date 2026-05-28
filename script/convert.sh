#!/usr/bin/env bash
set -euo pipefail

INPUT_CSV="$1"
COLUMNAR="$2"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SCHEMA="${SCRIPT_DIR}/schema_cpp.csv"
BIN="${REPO_ROOT}/build/src/columnar_db"

"${BIN}" convert "${INPUT_CSV}" "${SCHEMA}" "${COLUMNAR}"