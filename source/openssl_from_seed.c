#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define SEEDBYTES 32

void clean_hex(char *s) {
    char *d = s;
    while (*s) {
        if (*s != ':' && !isspace((unsigned char)*s))
            *d++ = *s;
        s++;
    }
    *d = '\0';
}

int hex_to_bin(const char *hex, uint8_t *out, size_t outlen) {
    size_t len = strlen(hex);
    if (len % 2 != 0)
        return -1;
    for (size_t i = 0; i < len / 2 && i < outlen; i++) {
        unsigned int byte;
        if (sscanf(hex + 2 * i, "%02x", &byte) != 1)
            return -1;
        out[i] = (uint8_t)byte;
    }
    return (int)(len / 2);
}

int save_bin(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "❌ Nepodarilo sa zapísať %s\n", path);
        return -1;
    }
    fwrite(data, 1, len, f);
    fclose(f);
    printf("  → uložené %s (%zu bajtov)\n", path, len);
    return 0;
}

int main(void) {
    char seed_hex[2 * SEEDBYTES + 1] = {0};
    uint8_t seed_bin[SEEDBYTES];
    FILE *f;
    int have_seed = 0;

    printf("=== OpenSSL from Seed (ML-DSA-44) ===\n");

    // načítaj seed
    if ((f = fopen("keys/app_seed.bin", "rb")) != NULL) {
        if (fread(seed_bin, 1, SEEDBYTES, f) == SEEDBYTES) {
            for (int i = 0; i < SEEDBYTES; i++)
                sprintf(seed_hex + 2 * i, "%02x", seed_bin[i]);
            have_seed = 1;
            printf("Načítaný seed z keys/app_seed.bin\n");
        }
        fclose(f);
    } else if ((f = fopen("keys/app_seed.hex", "r")) != NULL) {
        if (fgets(seed_hex, sizeof(seed_hex), f)) {
            for (int i = 0; seed_hex[i]; i++)
                if (isspace((unsigned char)seed_hex[i]))
                    seed_hex[i] = '\0';
            have_seed = 1;
            printf("Načítaný seed z keys/app_seed.hex\n");
        }
        fclose(f);
    }

    if (!have_seed) {
        fprintf(stderr, "❌ Seed sa nenašiel (chýba app_seed.bin/hex)\n");
        return 1;
    }

    // 1️⃣ openssl genpkey (bez system(), cez pipe)
    char cmd[600];
    snprintf(cmd, sizeof(cmd),
             "openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:%s", seed_hex);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "❌ Nepodarilo sa spustiť openssl genpkey\n");
        return 1;
    }

    FILE *outpem = fopen("keys/openssl_key.pem", "wb");
    if (!outpem) {
        fprintf(stderr, "❌ Nepodarilo sa otvoriť keys/openssl_key.pem na zápis\n");
        pclose(pipe);
        return 1;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0)
        fwrite(buf, 1, n, outpem);
    fclose(outpem);
    pclose(pipe);

    printf("✅ Vytvorený PEM: keys/openssl_key.pem\n");

    // 2️⃣ Spusti openssl pkey -text -noout
    pipe = popen("openssl pkey -in keys/openssl_key.pem -text -noout", "r");
    if (!pipe) {
        fprintf(stderr, "❌ Nepodarilo sa spustiť openssl pkey\n");
        return 1;
    }

    char *text = NULL;
    size_t text_size = 0;
    while (fgets(buf, sizeof(buf), pipe)) {
        size_t len = strlen(buf);
        char *tmp = realloc(text, text_size + len + 1);
        if (!tmp) {
            fprintf(stderr, "❌ Nedostatok pamäte\n");
            pclose(pipe);
            free(text);
            return 1;
        }
        text = tmp;
        memcpy(text + text_size, buf, len);
        text_size += len;
        text[text_size] = '\0';
    }
    pclose(pipe);

    if (!text || text_size == 0) {
        fprintf(stderr, "❌ OpenSSL nevygeneroval žiadny výstup\n");
        return 1;
    }

    FILE *fdump = fopen("keys/openssl_key_dump.txt", "w");
    if (fdump) {
        fwrite(text, 1, text_size, fdump);
        fclose(fdump);
    }

    // 3️⃣ Extrakcia sekcií seed:, priv:, pub:
    const char *names[] = {"seed:", "priv:", "pub:"};
    const char *out_files[] = {
        "keys/openssl_seed.bin",
        "keys/openssl_secretkey.bin",
        "keys/openssl_publickey.bin"
    };

    for (int nidx = 0; nidx < 3; nidx++) {
        const char *start = strstr(text, names[nidx]);
        if (!start) {
            printf("⚠ Nenájdený blok %s\n", names[nidx]);
            continue;
        }

        // posuň sa za názov a nový riadok
        const char *p = strchr(start, '\n');
        if (!p) continue;
        p++;

        char block[65536] = {0};
        size_t bi = 0;

        // zbieraj hex po ďalší nadpis alebo koniec
        while (*p) {
            if (strncmp(p, "seed:", 5) == 0 ||
                strncmp(p, "priv:", 5) == 0 ||
                strncmp(p, "pub:", 4) == 0)
                break;

            if (isxdigit((unsigned char)p[0]) || p[0] == ' ' || p[0] == '\t') {
                while (*p && *p != '\n' && bi < sizeof(block) - 1) {
                    block[bi++] = *p++;
                }
            }
            if (*p) p++;
        }

        block[bi] = '\0';
        clean_hex(block);

        size_t outlen = strlen(block) / 2;
        if (outlen == 0) {
            printf("⚠ Blok %s je prázdny\n", names[nidx]);
            continue;
        }

        uint8_t *out = malloc(outlen);
        if (!out) {
            fprintf(stderr, "❌ Nedostatok pamäte pri %s\n", names[nidx]);
            continue;
        }

        if (hex_to_bin(block, out, outlen) > 0)
            save_bin(out_files[nidx], out, outlen);
        else
            printf("⚠ Chyba dekódovania %s\n", names[nidx]);

        free(out);
    }

    free(text);
    printf("\n✅ Hotovo. Výstupy uložené do priečinka keys/\n");
    return 0;
}
