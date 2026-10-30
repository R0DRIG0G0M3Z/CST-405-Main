#!/usr/bin/env bash
set -u

COMPILER="${COMPILER:-./minicompiler}"
SPIM="${SPIM:-spim}"
TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT

expected_output() {
    case "$(basename "$1")" in
        labq14_array.cm) echo $'7' ;;
        labq15_functions.cm) echo $'10' ;;
        labq16_scope.cm) echo $'11\n5' ;;
        t3_01_arithmetic.cm) echo $'14\n20\n9\n3\n-20\n7' ;;
        t3_02_globals.cm) echo $'5\n12\n17' ;;
        t3_03_arrays.cm) echo $'30\n100\n60' ;;
        t3_04_functions.cm) echo $'6\n5\n7\n8' ;;
        t3_05_scope.cm) echo $'1\n2\n1' ;;
        t3_06_recursion.cm) echo $'120' ;;
        t3_07_forward_reference.cm) echo $'7' ;;
        t3_08_array_reference.cm) echo $'99' ;;
        t3_09_return_epilogue.cm) echo $'5' ;;
        t2_01_basics.cm) echo $'5\n10\n18' ;;
        t2_02_chained.cm) echo $'6\n21' ;;
        t2_03_comments.cm) echo $'42' ;;
        *) return 1 ;;
    esac
}

run_program() {
    local source=$1
    local base
    local assembly
    local expected
    local actual
    base=$(basename "${source%.cm}")
    assembly="$TEMP_DIR/$base.s"
    expected=$(expected_output "$source")

    if ! "$COMPILER" "$source" "$assembly" -q >/dev/null 2>&1; then
        echo "FAIL  $source did not compile"
        return 1
    fi

    local spim_args=()
    if [[ -n "${SPIM_EXCEPTION_FILE:-}" ]]; then
        spim_args=(-exception_file "$SPIM_EXCEPTION_FILE")
    fi

    actual=$("$SPIM" "${spim_args[@]}" -file "$assembly" 2>&1 |
        sed '/^SPIM Version/d;/^Copyright/d;/^All Rights/d;/^See the file/d;/^Loaded:/d')

    if [[ "$actual" != "$expected" ]]; then
        echo "FAIL  $source"
        echo "      expected: $(echo "$expected" | tr '\n' ' ')"
        echo "      actual:   $(echo "$actual" | tr '\n' ' ')"
        return 1
    fi

    echo "PASS  $source"
}

expect_failure() {
    local source=$1
    local assembly="$TEMP_DIR/$(basename "${source%.cm}").s"

    if "$COMPILER" "$source" "$assembly" -q >/dev/null 2>&1; then
        echo "FAIL  $source should have been rejected"
        return 1
    fi

    echo "PASS  $source was rejected as expected"
}

failed=0

for source in tests/labq*.cm tests/t3_*.cm; do
    run_program "$source" || failed=1
done

topic2=../../topic-2-minimal-compiler/compiler/tests
for source in "$topic2"/t2_01_basics.cm \
              "$topic2"/t2_02_chained.cm \
              "$topic2"/t2_03_comments.cm; do
    run_program "$source" || failed=1
done

expect_failure tests/error_semantic.cm || failed=1
expect_failure tests/error_too_many_args.cm || failed=1
expect_failure "$topic2/t2_04_errors_undeclared.cm" || failed=1
expect_failure "$topic2/t2_05_errors_syntax.cm" || failed=1

if [[ $failed -ne 0 ]]; then
    echo "Test suite failed."
    exit 1
fi

echo "All Topic 3 and Project 2 regression tests passed."
