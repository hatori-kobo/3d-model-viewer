#!/bin/bash

set -euo pipefail

if [[ -z "${project_dir-}" ]]; then
  echo "This script should not be executed directly. Instead execute" \
       "~/win_build.sh from the project directory, which sources this" \
       "project-local script."
  exit 1
fi

mkdir -p "$project_dir/build"

if [[ "$assets" -eq 1 ]]; then
    # Asset Compilation
    pushd "$project_dir" > /dev/null
    if [[ "$release" -eq 1 ]]; then
        asset_compiler -r
    else
        asset_compiler
    fi
    popd > /dev/null

    # Resource Compilation
    # -fo: Set RES file path
    rc -fo "$project_dir/icon.res" -nologo "$project_dir/icon.rc"
fi

# -I: Set include dir path
cl_flags+=(
    "-I$common_library_dir"
    "-I$vulkan_sdk_dir/include"
)

# -LIBPATH: Set library dir path
link_flags+=(
    "-LIBPATH:$common_library_dir/imgui/deps"
    "-LIBPATH:$vulkan_sdk_dir/lib"
)

# -link: Set linker flags
# -OUT: Set output file path
compile_exe=(
    "$cl"
    "$project_dir/$project_name.c"
    "$project_dir/icon.res"
    "${cl_flags[@]}"
    "-link"
    "${link_flags[@]}"
    "-OUT:$project_dir/build/$project_name.exe"
)
"${compile_exe[@]}"

clean_files+=(
)
if [[ "$clean" -eq 1 ]]; then
    pushd "$project_dir/build/" > /dev/null
    for file_pattern in "${clean_files[@]}"; do
        rm -f $file_pattern
    done
    popd > /dev/null
fi
