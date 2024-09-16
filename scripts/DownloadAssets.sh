#!/usr/bin/env bash

download_asset () {
    url=$1
    full_path=$2

    path="${full_path%/*}"
    filename="${full_path##*/}"

    org_name="${url##*/}"

    (cd ${path} && rm ${org_name})
    (cd ${path} && wget ${url} && mv ${org_name} ${filename})
}

download_asset vulkan-tutorial.com/images/texture.jpg ./assets/textures/texture.jpg
download_asset https://learnopengl.com/img/textures/container.jpg assets/textures/container.jpg