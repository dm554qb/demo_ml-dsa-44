#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "api.h"
#include "sign.h"
#include "params.h"
#include "packing.h"
#include "fips202.h"
#include "randombytes.h"

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif


/*
 * genkey.c - generuje ML-DSA-44 kluce a vypise 32-bajtovy seed
 *
 * Vystup:
 *  - keys/app_publickey.bin  (1312 bajtov)
 *  - keys/app_secretkey.bin  (2560 bajtov)
 *  - seed (vypisany do konzoly v tvare vhodnom pre OpenSSL)
 */

int main(void) {
    uint8_t pk[PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[PQCLEAN_MLDSA44_CLEAN_CRYPTO_SECRETKEYBYTES];
    uint8_t seed[SEEDBYTES];
    uint8_t seedbuf[2 * SEEDBYTES + CRHBYTES];
    uint8_t tr[TRBYTES];
    const uint8_t *rho, *rhoprime, *key;

    polyvecl mat[K];
    polyvecl s1, s1hat;
    polyveck s2, t1, t0;

    /* ---- 1. Vygeneruj 32-bajtovy seed ---- */
    randombytes(seed, SEEDBYTES);

    /* Vytlac seed v hex (lowercase, bez medzier) - presne tak, ako ho potrebuje OpenSSL */
    char seed_hex[SEEDBYTES * 2 + 1];
    for (int i = 0; i < SEEDBYTES; i++) {
        sprintf(&seed_hex[i * 2], "%02x", seed[i]);
    }
    seed_hex[SEEDBYTES * 2] = '\0';

    printf("Vygenerovany 32-bajtovy seed (hex, 64 znakov):\n%s\n\n", seed_hex);
    printf("Pouzi tento prikaz na vygenerovanie presne toho isteho kluca v OpenSSL:\n");
    printf("openssl genpkey -algorithm ML-DSA-44 -out keys/openssl_app_key.pem -pkeyopt seed:%s\n", seed_hex);
    printf("openssl pkey -in keys/openssl_app_key.pem -out keys/openssl_app_sk.pem\n");
    printf("openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem\n");

    /* ---- 2. Pouzi seed na odvodenie rho, rhoprime, key (rovnako ako PQClean) ---- */
    memcpy(seedbuf, seed, SEEDBYTES);
    seedbuf[SEEDBYTES + 0] = K;
    seedbuf[SEEDBYTES + 1] = L;
    /* shake256(output, outlen, input, inlen) */
    shake256(seedbuf, 2 * SEEDBYTES + CRHBYTES, seedbuf, SEEDBYTES + 2);
    rho = seedbuf;
    rhoprime = rho + SEEDBYTES;
    key = rhoprime + CRHBYTES;

    /* ---- 3. Generovanie klucov (rovnake ako v PQClean) ---- */
    PQCLEAN_MLDSA44_CLEAN_polyvec_matrix_expand(mat, rho);
    PQCLEAN_MLDSA44_CLEAN_polyvecl_uniform_eta(&s1, rhoprime, 0);
    PQCLEAN_MLDSA44_CLEAN_polyveck_uniform_eta(&s2, rhoprime, L);

    s1hat = s1;
    PQCLEAN_MLDSA44_CLEAN_polyvecl_ntt(&s1hat);
    PQCLEAN_MLDSA44_CLEAN_polyvec_matrix_pointwise_montgomery(&t1, mat, &s1hat);
    PQCLEAN_MLDSA44_CLEAN_polyveck_reduce(&t1);
    PQCLEAN_MLDSA44_CLEAN_polyveck_invntt_tomont(&t1);

    PQCLEAN_MLDSA44_CLEAN_polyveck_add(&t1, &t1, &s2);
    PQCLEAN_MLDSA44_CLEAN_polyveck_caddq(&t1);
    PQCLEAN_MLDSA44_CLEAN_polyveck_power2round(&t1, &t0, &t1);
    PQCLEAN_MLDSA44_CLEAN_pack_pk(pk, rho, &t1);

    shake256(tr, TRBYTES, pk, PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES);
    PQCLEAN_MLDSA44_CLEAN_pack_sk(sk, rho, tr, key, &t0, &s1, &s2);

    /* ---- 4. Zapis do suborov (vytvor priecinok keys, ak neexistuje) ---- */
#ifdef _WIN32
    _mkdir("keys");
#else
    system("mkdir -p keys");
#endif

    FILE *fpk = fopen("keys/app_pk.bin", "wb");
    FILE *fsk = fopen("keys/app_sk.bin", "wb");
    if (!fpk || !fsk) {
        perror("Chyba pri otvarani suborov");
        return EXIT_FAILURE;
    }

    fwrite(pk, 1, sizeof(pk), fpk);
    fwrite(sk, 1, sizeof(sk), fsk);
    fclose(fpk);
    fclose(fsk);

    printf("Kluc bol uspesne vygenerovany a ulozeny do:\n");
    printf("  keys/app_pk.bin  (%zu bajtov)\n", sizeof(pk));
    printf("  keys/app_sk.bin  (%zu bajtov)\n\n", sizeof(sk));
    printf("Uloz si uvedeny seed (%s) - s nim OpenSSL vygeneruje presne ten isty par klucov.\n", seed_hex);

    /* uloz seed ako raw bin a ako text hex do priecinka keys */
    FILE *fseedbin = fopen("keys/app_seed.bin", "wb");
    FILE *fseedtxt = fopen("keys/app_seed.hex", "w");
    if (fseedbin && fseedtxt) {
        fwrite(seed, 1, SEEDBYTES, fseedbin);
        fprintf(fseedtxt, "%s\n", seed_hex);
        fclose(fseedbin);
        fclose(fseedtxt);
        printf("Seed ulozeny aj do keys/app_seed.bin (raw %d bajtov) a keys/app_seed.hex (hex)\n", SEEDBYTES);
    } else {
        if (fseedbin) fclose(fseedbin);
        if (fseedtxt) fclose(fseedtxt);
        fprintf(stderr, "Nepodarilo sa ulozit seed do keys/ (skontroluj prava).\n");
    }

    return EXIT_SUCCESS;
}
