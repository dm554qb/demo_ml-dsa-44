#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "api.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Použitie: %s <subor_na_podpisanie> <subor_so_sukromnym_klucom>\n", argv[0]);
        fprintf(stderr, "Príklad: %s files/test_bin.bin keys/app_secretkey.bin\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_file = argv[1];
    const char *secretkey_file = argv[2];

    FILE *fin = fopen(input_file, "rb");
    if (!fin) {
        perror("Chyba pri otváraní vstupného súboru");
        return EXIT_FAILURE;
    }

    FILE *fsk = fopen(secretkey_file, "rb");
    if (!fsk) {
        perror("Chyba pri otváraní súkromného kľúča");
        fclose(fin);
        return EXIT_FAILURE;
    }

    // Načítanie obsahu súboru
    fseek(fin, 0, SEEK_END);
    size_t mlen = ftell(fin);
    rewind(fin);

    uint8_t *message = malloc(mlen);
    if (!message) {
        fprintf(stderr, "Chyba: nedostatok pamäte.\n");
        fclose(fin);
        fclose(fsk);
        return EXIT_FAILURE;
    }

    fread(message, 1, mlen, fin);
    fclose(fin);

    // Načítanie súkromného kľúča
    uint8_t sk[PQCLEAN_MLDSA44_CLEAN_CRYPTO_SECRETKEYBYTES];
    size_t read_bytes = fread(sk, 1, sizeof(sk), fsk);
    fclose(fsk);

    if (read_bytes != sizeof(sk)) {
        fprintf(stderr, "Chyba: nesprávna veľkosť súkromného kľúča (%zu / %zu bajtov)\n",
                read_bytes, sizeof(sk));
        free(message);
        return EXIT_FAILURE;
    }

    // Vytvorenie podpisu
    uint8_t sig[PQCLEAN_MLDSA44_CLEAN_CRYPTO_BYTES];
    size_t siglen = 0;

    if (PQCLEAN_MLDSA44_CLEAN_crypto_sign_signature(sig, &siglen, message, mlen, sk) != 0) {
        fprintf(stderr, "Chyba: podpis sa nepodarilo vytvoriť.\n");
        free(message);
        return EXIT_FAILURE;
    }

    // Uloženie podpisu do súboru
    FILE *fsig = fopen("keys/app_signature.bin", "wb");
    if (!fsig) {
        perror("Chyba pri ukladaní podpisu");
        free(message);
        return EXIT_FAILURE;
    }

    fwrite(sig, 1, siglen, fsig);
    fclose(fsig);

    printf("✅ Súbor '%s' bol úspešne podpísaný.\n", input_file);
    printf("🔑 Použitý súkromný kľúč: %s\n", secretkey_file);
    printf("📄 Podpis uložený ako 'signature.bin' (%zu bajtov)\n", siglen);

    free(message);
    return EXIT_SUCCESS;
}
