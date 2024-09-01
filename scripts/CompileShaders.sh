#!/usr/bin/env bash

SPIRV_DIR="assets/spirv"

mkdir -p ${SPIRV_DIR}

SHADER_FILEPATHS=$(find "assets/shaders" -iname '*.vert' -o -iname '*.frag')

for filepath in $SHADER_FILEPATHS; do
    filename=${filepath##*/}

    raw_filename=${filename%.*}
    extension=${filename##*.}

    result_path=${SPIRV_DIR}
    result_path="${result_path}/${raw_filename}"

    if [ "$extension" = "vert" ]; then
        result_path="${result_path}Vert.spv"
    elif [ "$extension" = "frag" ]; then
        result_path="${result_path}Frag.spv"
    fi

    glslc ${filepath} -o ${result_path}
done