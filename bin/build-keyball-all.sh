#!/usr/bin/env bash

set -u

id=$(date "+%Y%m%d_%H%M%S")
repo_root=$(cd "$(dirname "$0")/.." && pwd)
qmk_home=${QMK_HOME:-$PWD}
build_dir="${repo_root}/build"
logdir="${build_dir}/build_log/${id}"

keyboards=()
keyboards+=(keyball39)
keyboards+=(keyball44)
keyboards+=(keyball46)
keyboards+=(keyball61)
keyboards+=(one47)

keymaps=()
keymaps+=(test)
keymaps+=(default)
keymaps+=(via)

mkdir -p "${logdir}"

for kb in "${keyboards[@]}" ; do
  tmpmaps=(${keymaps[@]})
  # Add special keymaps for keyball46
  if [ $kb = keyball46 ] ; then
    tmpmaps+=(test_Left test_Both)
    tmpmaps+=(via_Left via_Both)
  fi
  for km in "${tmpmaps[@]}" ; do
    ( python3 "${repo_root}/scripts/build_firmware.py" --qmk-home "${qmk_home}" --keyboard "${kb}" --keymap "${km}" 2>&1 | tee "${logdir}/${kb}-${km}.log" | LANG=C.utf-8 ts "[${kb}:${km}]" ) &
  done
done

wait

"${repo_root}/bin/hexsize.sh" "${build_dir}"/keyball_*.hex | tee "${logdir}/size.tsv"
