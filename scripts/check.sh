#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TEST_BIN="$(mktemp /private/tmp/air-stone-test.XXXXXX)"
trap 'rm -f "$TEST_BIN"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror tests/pump_controller_test.cpp -o "$TEST_BIN"
"$TEST_BIN"
printf 'Pump scheduler and safety tests passed.\n'
esphome config air-stone-controller.yaml >/dev/null
