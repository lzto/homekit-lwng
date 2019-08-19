add_library (wolfssl STATIC
    wolfcrypt/src/hash.c
    wolfcrypt/src/hmac.c
    wolfcrypt/src/ed25519.c
    wolfcrypt/src/curve25519.c
    wolfcrypt/src/sha256.c
    wolfcrypt/src/sha512.c
    wolfcrypt/src/poly1305.c
    wolfcrypt/src/chacha.c
    wolfcrypt/src/chacha20_poly1305.c
    wolfcrypt/src/random.c
    wolfcrypt/src/srp.c
    wolfcrypt/src/ge_operations.c
    wolfcrypt/src/fe_operations.c
    wolfcrypt/src/integer.c
    wolfcrypt/src/memory.c
    wolfcrypt/src/coding.c
    )

include_directories( .)

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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    ${WOLFSSL_C_FLAGS}")


