#!/bin/bash

TMPFILE=file.tmp
STDOUTFILE=out.tmp
TESTDOC=${TESTS_DIR}/docs/HUN.pdf
TEST_ENCDOC=${TESTS_DIR}/docs/enc_test.pdf

DIFF="diff"
RM="rm -f"
BZIP2="bzip2 -k"
BUNZIP2="$BZIP2 -d"

# exit codes as defined in pdf_error_tracker.h
readonly CODE_INIT_ERROR=0x80
readonly CODE_RUNTIME_POPPLER=0x40
readonly CODE_RUNTIME_LIBPNG=0x20
readonly CODE_RUNTIME_LIBLEPT=0x10
readonly CODE_RUNTIME_FONTMAP=0x08
readonly CODE_RUNTIME_OK=0


# set the pdftoedn binary
test_start () {
    if [ ! -x "$PDFTOEDN" ]; then
        export PDFTOEDN=`which pdftoedn`

        if [ -z "$PDFTOEDN" ]; then
            echo "No pdftoedn binary found"
            exit -1
        fi

        echo "** Missing build pdftoedn exec - Using $PDFTOEDN **"
    fi
}

# remove contents of :filename and :versions hashes from data output
# so diff can compare everything else
filter_meta () {
    local SRC="$1"
    local DST="$2"
    cat "$SRC" | sed 's/:versions {[a-z0-9:\ \.,_"]*}/:versions {}/' | sed 's/:filename "[a-zA-Z0-9/\_\.\-]*"/:filename ""/' > "$DST"
}

# output and execute the command
run_cmd () {
    local cmd="$@"

    # output the command, run it, grab its return code and std-output
    echo $cmd
    $cmd > $STDOUTFILE
    local status=$?
    cat $STDOUTFILE
    return $status
}

flag_set () {
    local code=$1
    local pattern=$2
    local val=$(($code & $pattern))
    [ $val != 0 ]
}

check_stdout () {
    local expected_str="$@"
    [ "`grep "$expected_str" "$STDOUTFILE"`" != "" ]
}

# cleanup
test_end () {
    [ -f "$TMPFILE" ] && $RM "$TMPFILE"
}
