/*
 * Local wolfSSL user settings guard file.
 * Ensures configuration macros are only defined when missing to avoid
 * redefinition warnings when the build system also supplies them.
 */

#ifndef WOLFCRYPT_HAVE_SRP
#define WOLFCRYPT_HAVE_SRP
#endif

#ifndef HAVE_CHACHA
#define HAVE_CHACHA
#endif

#ifndef HAVE_POLY1305
#define HAVE_POLY1305
#endif

#ifndef WOLFSSL_BASE64_ENCODE
#define WOLFSSL_BASE64_ENCODE
#endif

#ifndef NO_SESSION_CACHE
#define NO_SESSION_CACHE
#endif

#ifndef RSA_LOW_MEM
#define RSA_LOW_MEM
#endif

#ifndef HAVE_HKDF
#define HAVE_HKDF
#endif

#ifndef WOLFSSL_SHA512
#define WOLFSSL_SHA512
#endif
