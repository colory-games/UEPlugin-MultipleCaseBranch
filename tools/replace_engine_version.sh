#!/usr/bin/env bash
# require: bash version >= 4
# usage example: bash replace_engine_version.sh 5.0.0
set -eEu


SUPPORTED_VERSIONS=(
    "4.27"
    "5.0"
)

function usage() {
    
}

if [ $# -ne 2 ]; then
    usage
    exit 1
fi

engine_version=${1}
source_dir=${2}

supported=0
for v in "${SUPPORTED_VERSIONS[@]}"; do
    if [ ${v} = ${engine_version} ]; then
        supported=1
    fi
done
if [ ${supported} -eq 0 ]; then
    echo "${engine_version} is not supported."
    echo "Supported version is ${SUPPORTED_VERSIONS[@]}"
    exit 1
fi

for file in `find -name "*.uplugin" ${source_dir}`; do
    sed -i s/"EngineVersion": "5.0.0",/"EngineVersion": ${engine_version}/g ${file}
done
