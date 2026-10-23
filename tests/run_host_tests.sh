#!/bin/sh
set -eu
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/bt2ps2-tests.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    -I"$repo_dir/include" "$repo_dir/tests/hid_input_test.cpp" \
    "$repo_dir/main/hid_report.cpp" "$repo_dir/main/input_state.cpp" -o "$test_dir/hid_input_test"
"$test_dir/hid_input_test"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    -I"$repo_dir/include" "$repo_dir/tests/input_bridge_test.cpp" \
    "$repo_dir/main/input_bridge.cpp" -o "$test_dir/input_bridge_test"
"$test_dir/input_bridge_test"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    -I"$repo_dir/include" "$repo_dir/tests/ps2_mouse_test.cpp" -o "$test_dir/ps2_mouse_test"
"$test_dir/ps2_mouse_test"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g \
    -I"$repo_dir/tests/stubs" -I"$repo_dir/include" "$repo_dir/tests/ps2_protocol_test.cpp" \
    "$repo_dir/tests/stubs/ps2_runtime.cpp" "$repo_dir/main/ps2_bus.cpp" \
    "$repo_dir/main/ps2_keyboard.cpp" "$repo_dir/main/ps2_mouse.cpp" "$repo_dir/main/input_bridge.cpp" \
    -o "$test_dir/ps2_protocol_test"
"$test_dir/ps2_protocol_test"
