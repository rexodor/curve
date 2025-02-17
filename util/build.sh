#!/usr/bin/env bash

# Copyright (C) 2021 Jingli Chen (Wine93), NetEase Inc.

############################  GLOBAL VARIABLES

g_list=0
g_depend=0
g_target=""
g_release=0
g_build_opts=(
    "--define=with_glog=true"
    "--define=libunwind=true"
    "--copt -DHAVE_ZLIB=1"
    "--copt -DGFLAGS_NS=google"
    "--copt -DUSE_BTHREAD_MUTEX"
)

g_os="debian9"

############################  BASIC FUNCTIONS
msg() {
    printf '%b' "$1" >&2
}

success() {
    msg "\33[32m[✔]\33[0m ${1}${2}"
}

die() {
    msg "\33[31m[✘]\33[0m ${1}${2}"
    exit 1
}

print_title() {
    local delimiter=$(printf '=%.0s' {1..20})
    msg "$delimiter [$1] $delimiter\n"
}

############################ FUNCTIONS
usage () {
    cat << _EOC_
Usage:
    build.sh --list
    build.sh --only=target
Examples:
    build.sh --only=//src/chunkserver:chunkserver
    build.sh --only=src/*
    build.sh --only=test/*
    build.sh --only=test/chunkserver
_EOC_
}

get_options() {
    local args=`getopt -o ldorh --long list,dep:,only:,os:,release: -n "$0" -- "$@"`
    eval set -- "${args}"
    while true
    do
        case "$1" in
            -l|--list)
                g_list=1
                shift 1
                ;;
            -d|--dep)
                g_depend=$2
                shift 2
                ;;
            -o|--only)
                g_target=$2
                shift 2
                ;;
            -r|--release)
                g_release=$2
                shift 2
                ;;
            --os)
                g_os=$2
                shift 2
                ;;
            -h)
                usage
                exit 1
                ;;
            --)
                shift
                break
                ;;
            *)
                exit 1
                ;;
        esac
    done
}

list_target() {
    git submodule update --init -- nbd
    if [ $? -ne 0 ]
    then
        echo "submodule init failed"
        exit
    fi
    print_title " SOURCE TARGETS "
    bazel query 'kind("cc_binary", //src/...)'
    bazel query 'kind("cc_binary", //tools/...)'
    bazel query 'kind("cc_binary", //nebd/src/...)'
    bazel query 'kind("cc_binary", //nbd/src/...)'
    print_title " TEST TARGETS "
    bazel query 'kind("cc_(test|binary)", //test/...)'
    bazel query 'kind("cc_(test|binary)", //nebd/test/...)'
    bazel query 'kind("cc_(test|binary)", //nbd/test/...)'
}

get_target() {
    bazel query 'kind("cc_(test|binary)", //...)' | grep -E "$g_target"
}

build_target() {
    git submodule update --init -- nbd
    if [ $? -ne 0 ]
    then
        echo "submodule init failed"
        exit
    fi
    local targets
    local tag=$(git describe --tags --abbrev=0)
    local commit_id=$(git rev-parse --short HEAD)
    local version="${tag}+${commit_id}"
    declare -A result
    if [ $g_release -eq 1 ]; then
        g_build_opts+=("--compilation_mode=opt --copt -g")
        version="${version}+release"
        echo "release" > .BUILD_MODE
    else
        g_build_opts+=("--compilation_mode=dbg")
        version="${version}+debug"
        echo "debug" > .BUILD_MODE
    fi
    g_build_opts+=("--copt -DCURVEVERSION=${version}")

    if [ `gcc -dumpversion | awk -F'.' '{print $1}'` -gt 6 ]; then
        g_build_opts+=("--config=gcc7-later")
    fi

    for target in `get_target`
    do
        bazel build ${g_build_opts[@]} $target
        local ret="$?"
        targets+=("$target")
        result["$target"]=$ret
        if [ "$ret" -ne 0 ]; then
            break
        fi
    done

    echo ""
    print_title " BUILD SUMMARY "
    for target in "${targets[@]}"
    do
        if [ "${result[$target]}" -eq 0 ]; then
            success "$target ${version}\n"
        else
            die "$target ${version}\n"
        fi
    done
}


build_requirements() {
    g_aws_sdk_root="thirdparties/aws"
    (cd ${g_aws_sdk_root} && make)
    g_etcdclient_root="thirdparties/etcdclient"
    (cd ${g_etcdclient_root} && make clean && make all)
}

main() {
    get_options "$@"

    if [ "$g_list" -eq 1 ]; then
        list_target
    elif [[ "$g_target" == "" && "$g_depend" -ne 1 ]]; then
        usage
        exit 1
    else
        if [ "$g_depend" -eq 1 ]; then
            build_requirements
        fi
        build_target
    fi
}

############################  MAIN()
main "$@"
