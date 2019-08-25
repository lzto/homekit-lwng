cmake_minimum_required(VERSION 3.0)

project (homekit)

add_subdirectory(cJSON)
add_subdirectory(libmdnsd)
add_subdirectory(wolfssl)
add_subdirectory(http-parser)

include_directories(include cJSON libmdnsd wolfssl http-parser .)

set( SOURCES
    "src/json.c"
    "src/pairing.c"
    "src/storage.c"
    "src/debug.c"
    "src/accessories.c"
    "src/base64.c"
    "src/port.c"
    "src/query_params.c"
    "src/tlv.c"
    "src/server.c"
    "src/crypto.c"
    "src/addr.c"
    "src/cam_backend.c"
    "src/test.c"
    )

set(WOLFSSL_C_FLAGS
    "-DWOLFCRYPT_HAVE_SRP \
    -DWOLFSSL_SHA512 \
    -DWOLFSSL_BASE64_ENCODE \
    -DNO_MD5 \
    -DNO_SHA \
    -DHAVE_HKDF \
    -DHAVE_CHACHA \
    -DHAVE_POLY1305 \
    -DHAVE_ED25519 \
    -DHAVE_CURVE25519 \
    -DNO_SESSION_CACHE \
    -DRSA_LOW_MEM \
    -DGCM_SMALL \
    -DUSE_SLOW_SHA512 \
    -DWOLFCRYPT_ONLY \
    -DWC_NO_HARDEN"
    )


add_executable (homekit cJSON libmdnsd wolfssl http-parser ${SOURCES} )

target_link_libraries(homekit cjson mdnsd wolfssl http-parser)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
${WOLFSSL_C_FLAGS} \
-std=c99 \
-D_GNU_SOURCE")
#-g -fsanitize=address \
#-DHOMEKIT_DEBUG")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
-lpthread ")

