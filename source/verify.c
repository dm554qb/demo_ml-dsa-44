#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Pouzitie: %s <subor_na_overenie> <publickey.bin> <signature.bin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_file = argv[1];
    const char *pubkey_file = argv[2];
    const char *sig_file = argv[3];

    // ---- Načítanie vstupného súboru ----
    FILE *fin = fopen(input_file, "rb");
    if (!fin) {
        perror("Chyba pri otváraní vstupného súboru");
        return EXIT_FAILURE;
    }
    fseek(fin, 0, SEEK_END);
    size_t mlen = ftell(fin);
    rewind(fin);
    uint8_t *message = malloc(mlen);
    if (!message) {
        fprintf(stderr, "Chyba alokácie pamäte pre správu\n");
        fclose(fin);
        return EXIT_FAILURE;
    }
    fread(message, 1, mlen, fin);
    fclose(fin);

    // ---- Načítanie podpisu ----
    FILE *fsig = fopen(sig_file, "rb");
    if (!fsig) {
        perror("Chyba pri otváraní podpisového súboru");
        free(message);
        return EXIT_FAILURE;
    }
    uint8_t signature[PQCLEAN_MLDSA44_CLEAN_CRYPTO_BYTES];
    size_t siglen = fread(signature, 1, sizeof(signature), fsig);
    fclose(fsig);
    if (siglen != PQCLEAN_MLDSA44_CLEAN_CRYPTO_BYTES) {
        fprintf(stderr, "Neplatná dĺžka podpisu (%zu bajtov, očakáva sa %d)\n",
                siglen, PQCLEAN_MLDSA44_CLEAN_CRYPTO_BYTES);
        free(message);
        return EXIT_FAILURE;
    }

    // ---- Načítanie verejného kľúča ----
    FILE *fpk = fopen(pubkey_file, "rb");
    if (!fpk) {
        perror("Chyba pri otváraní verejného kľúča");
        free(message);
        return EXIT_FAILURE;
    }
    uint8_t pk[PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES];
    size_t pklen = fread(pk, 1, sizeof(pk), fpk);
    fclose(fpk);
    if (pklen != PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES) {
        fprintf(stderr, "Neplatná dĺžka verejného kľúča (%zu bajtov, očakáva sa %d)\n",
                pklen, PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES);
        free(message);
        return EXIT_FAILURE;
    }

    // ---- Overenie podpisu ----
    int ret = PQCLEAN_MLDSA44_CLEAN_crypto_sign_verify(
        signature, siglen, message, mlen, pk);

    if (ret == 0) {
        printf("✅ Podpis je platný pre súbor: %s\n", input_file);
    } else {
        printf("❌ Podpis NIE JE platný pre súbor: %s\n", input_file);
    }

    free(message);
    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
