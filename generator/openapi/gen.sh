#!/bin/bash
    #generate -g cpp-request \
    #generate -g cpp-qt-qhttpengine-server \

mvn package
ver=1.0.0
java -cp $PWD/target/cpp-request-openapi-generator-$ver.jar:$HOME/.nix-profile/share/java/openapi-generator-cli-7.7.0.jar \
    org.openapitools.codegen.OpenAPIGenerator \
    generate -g cpp-request \
    -i jellyfin-openapi-stable.json \
    --inline-schema-name-mappings BaseItemDto_ImageBlurHashes=BaseItemDtoImageBlurHashes,BaseItemPerson_ImageBlurHashes=BaseItemPersonImageBlurHashes \
    -p namespace=jellyfin \
    --package-name jellyfin-cpp \
    -o ../../service/jellyfin
