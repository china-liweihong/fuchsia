#!/bin/bash
# Copyright 2019 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Command to publish a package to make is accessible to a Fuchsia device.

# note: set -e is not used in order to have custom error handling.
set -u

# Source common functions
SCRIPT_SRC_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"

# Fuchsia command common functions.
# shellcheck disable=SC1090
source "${SCRIPT_SRC_DIR}/fuchsia-common.sh" || exit $?

FUCHSIA_SDK_PATH="$(get-fuchsia-sdk-dir)"
FUCHSIA_IMAGE_WORK_DIR="$(get-fuchsia-sdk-data-dir)"

usage () {
  echo "Usage: $0 <files.far>"
  echo "  [--work-dir <working directory to store image assets>]"
  echo "    Defaults to ${FUCHSIA_IMAGE_WORK_DIR}"
}

POSITIONAL=()

# Parse command line
while (( "$#" )); do
case $1 in
    --work-dir)
    shift
    FUCHSIA_IMAGE_WORK_DIR="${1}"
    ;;
    -*)
    if [[ "${#POSITIONAL[@]}" -eq 0 ]]; then
      echo "Unknown option ${1}"
      usage
      exit 1
    else
      POSITIONAL+=("${1}")
    fi
    ;;
    *)
      POSITIONAL+=("${1}")
    ;;
esac
shift
done

"${FUCHSIA_SDK_PATH}/tools/pm" publish  -a -r "${FUCHSIA_IMAGE_WORK_DIR}/packages/amber-files" -f "${POSITIONAL[@]}";
