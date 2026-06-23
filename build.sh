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
    # Shader Compilation
    # -D: Set preprocessor macro
    # -E: Set entrypoint name
    # -T: Set shader target profile
    # -Fo: Set output file path
    # -vkbr a b c d: Assign HLSL register (a) in space (b) to binding (c) in descriptor set (d)
    compile_vulkan_vs=(
        "$vulkan_dxc"
        "$project_dir/shaders.hlsl"
        "${vulkan_dxc_flags[@]}"
        "-DVULKAN"
        "-E" "vs"
        "-T" "vs_6_0"
        "-Fo" "$project_dir/build/vulkan_vs.spv"
        "-vkbr" "s0" "0" "0" "0"
        "-vkbr" "b1" "0" "1" "0"
        "-vkbr" "t2" "0" "2" "0"
        "-vkbr" "t3" "0" "3" "0"
        "-vkbr" "t4" "0" "4" "0"
        "-vkbr" "t5" "0" "5" "0"
        "-vkbr" "t6" "1" "6" "0"
    )
    compile_vulkan_ps=(
        "$vulkan_dxc"
        "$project_dir/shaders.hlsl"
        "${vulkan_dxc_flags[@]}"
        "-DVULKAN"
        "-E" "ps"
        "-T" "ps_6_0"
        "-Fo" "$project_dir/build/vulkan_ps.spv"
        "-vkbr" "s0" "0" "0" "0"
        "-vkbr" "b1" "0" "1" "0"
        "-vkbr" "t2" "0" "2" "0"
        "-vkbr" "t3" "0" "3" "0"
        "-vkbr" "t4" "0" "4" "0"
        "-vkbr" "t5" "0" "5" "0"
        "-vkbr" "t6" "1" "6" "0"
    )
    "${compile_vulkan_vs[@]}"
    "${compile_vulkan_ps[@]}"

    # Asset Compilation
    pushd "$project_dir" > /dev/null
    asset_compiler
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
