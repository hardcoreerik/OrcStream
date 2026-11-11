#!/bin/sh
# Runs in current working directory; log is always written right here.
set -u
LOG="$PWD/orcstream-p1-test.log"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
{
  printf 'date: ';date -u
  printf 'working_directory: %s\n' "$PWD"
  uname -a
  gcc --version | head -1
  clang --version | head -1
  cmake --version | head -1
  command -v idf.py || echo 'ESP-IDF unavailable'
  for cc in gcc clang; do
    for cfg in Debug Release; do
      d="$PWD/build-p1-$cc-$cfg"
      echo "COMMAND: CC=$cc cmake -S $ROOT -B $d -DCMAKE_BUILD_TYPE=$cfg -DCMAKE_C_FLAGS=-Werror"
      CC="$cc" cmake -S "$ROOT" -B "$d" -DCMAKE_BUILD_TYPE="$cfg" -DCMAKE_C_FLAGS=-Werror || exit 1
      echo "COMMAND: cmake --build $d --parallel 2"
      cmake --build "$d" --parallel 2 || exit 1
      echo "COMMAND: ctest --test-dir $d --output-on-failure"
      ctest --test-dir "$d" --output-on-failure || exit 1
    done
  done
  d="$PWD/build-p1-sanitizer"
  echo "COMMAND: CC=clang cmake -S $ROOT -B $d -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -Werror'"
  CC=clang cmake -S "$ROOT" -B "$d" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -Werror' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' || exit 1
  cmake --build "$d" --parallel 2 || exit 1
  ctest --test-dir "$d" --output-on-failure || exit 1
  if clang -std=c99 -fsanitize=fuzzer,address,undefined -I"$ROOT/include" "$ROOT/tests/fuzz_decode.c" "$ROOT/src/orcstream.c" -o "$d/fuzz_decode"; then
    echo "COMMAND: $d/fuzz_decode -runs=20000 $ROOT/tests/corpus"
    "$d/fuzz_decode" -runs=20000 "$ROOT/tests/corpus" || exit 1
  else
    echo 'libFuzzer unavailable; not passed'
  fi
  echo "COMMAND: $PWD/build-p1-gcc-Release/orcstream_bench"
  "$PWD/build-p1-gcc-Release/orcstream_bench" || exit 1
  echo "COMMAND: nm -u $PWD/build-p1-gcc-Release/liborcstream.a"
  nm -u "$PWD/build-p1-gcc-Release/liborcstream.a"
} > "$LOG" 2>&1
status=$?
tail -35 "$LOG"
exit "$status"
