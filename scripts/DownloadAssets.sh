#!/usr/bin/env bash

download_asset () {
    url=$1
    full_path=$2

    path="${full_path%/*}"
    filename="${full_path##*/}"

    org_name="${url##*/}"

    mkdir -p ${path}

    (cd ${path} && rm ${org_name})
    (cd ${path} && wget ${url} && mv ${org_name} ${filename})
}

download_asset vulkan-tutorial.com/images/texture.jpg ./assets/textures/texture.jpg

download_asset https://learnopengl.com/img/textures/container.jpg assets/textures/container.jpg

download_asset https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/refs/heads/main/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf ./assets/gltf/DamagedHelmet/DamagedHelmet.gltf
download_asset https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/refs/heads/main/2.0/DamagedHelmet/glTF/DamagedHelmet.bin ./assets/gltf/DamagedHelmet/DamagedHelmet.bin
download_asset https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/refs/heads/main/2.0/DamagedHelmet/glTF/Default_albedo.jpg ./assets/gltf/DamagedHelmet/Default_albedo.jpg