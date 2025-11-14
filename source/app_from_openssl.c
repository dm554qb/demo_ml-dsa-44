#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #include <direct.h>   // _mkdir
#else
  #include <sys/stat.h> // mkdir
  #include <sys/types.h>
#endif

#include "api.h"   // PQCLEAN_MLDSA44_CLEAN_*_BYTES

#define SEEDBYTES 32

static void ensure_keys_dir(void) {
#ifdef _WIN32
    _mkdir("keys");
#else
    mkdir("keys", 0700);
#endif
}

static void clean_hex(char *s) {
    char *d = s;
    while (*s) {
        if (*s != ':' && !isspace((unsigned char)*s))
            *d++ = *s;
        s++;
    }
    *d = '\0';
}

static int hex_to_bin(const char *hex, uint8_t *out, size_t outlen) {
    size_t len = strlen(hex);
    if (len % 2 != 0)
        return -1;
    if (outlen < len / 2)
        return -1;

    for (size_t i = 0; i < len / 2; i++) {
        unsigned int byte;
        if (sscanf(hex + 2 * i, "%02x", &byte) != 1)
            return -1;
        out[i] = (uint8_t)byte;
    }
    return (int)(len / 2);
}

static int save_bin(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Nepodarilo sa zapisat %s\n", path);
        return -1;
    }
    fwrite(data, 1, len, f);
    fclose(f);
    printf("  -> ulozene %s (%zu bajtov)\n", path, len);
    return 0;
}

/* vyberie hex blok za danym nadpisom (napr. "seed:" / "priv:" / "pub:") */
static char *extract_block(const char *text, const char *label) {
    const char *start = strstr(text, label);
    if (!start) {
        return NULL;
    }

    /* posun na dalsi riadok za "seed:" / "priv:" / "pub:" */
    const char *p = strchr(start, '\n');
    if (!p) return NULL;
    p++;

    char *block = NULL;
    size_t bi = 0;

    while (*p) {
        /* koniec bloku: dalsi label alebo koniec textu */
        if (strncmp(p, "seed:", 5) == 0 ||
            strncmp(p, "priv:", 5) == 0 ||
            strncmp(p, "pub:", 4) == 0)
            break;

        /* zober riadky, kde je hex / medzera / tab */
        if (isxdigit((unsigned char)p[0]) || p[0] == ' ' || p[0] == '\t') {
            const char *line_start = p;
            while (*p && *p != '\n')
                p++;

            size_t line_len = (size_t)(p - line_start);
            char *tmp = realloc(block, bi + line_len + 2);
            if (!tmp) {
                free(block);
                return NULL;
            }
            block = tmp;
            memcpy(block + bi, line_start, line_len);
            bi += line_len;
            block[bi++] = '\n';
            block[bi] = '\0';
        }

        if (*p == '\n')
            p++;
    }

    if (!block) return NULL;
    clean_hex(block);
    return block;
}

int main(int argc, char *argv[]) {
    const char *pem_path = "keys/openssl_key.pem";
    if (argc == 2) {
        pem_path = argv[1];
    } else {
        printf("Pouzitie: %s <subor_pem>\n", argv[0]);
        printf("Ak nezadas argument, pouzije sa predvolene: keys/openssl_key.pem\n\n");
    }

    ensure_keys_dir();

    /* 1) zavolaj "openssl pkey -in ... -text -noout" a nacitaj vystup */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "openssl pkey -in \"%s\" -text -noout", pem_path);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "Nepodarilo sa spustit prikaz: %s\n", cmd);
        return 1;
    }

    char buf[4096];
    char *text = NULL;
    size_t text_size = 0;

    while (fgets(buf, sizeof(buf), pipe)) {
        size_t len = strlen(buf);
        char *tmp = realloc(text, text_size + len + 1);
        if (!tmp) {
            fprintf(stderr, "Nedostatok pamate\n");
            free(text);
            pclose(pipe);
            return 1;
        }
        text = tmp;
        memcpy(text + text_size, buf, len);
        text_size += len;
        text[text_size] = '\0';
    }
    pclose(pipe);

    if (!text || text_size == 0) {
        fprintf(stderr, "OpenSSL nevygeneroval ziaden vystup (je PEM subor OK?).\n");
        free(text);
        return 1;
    }

    /* volitelne: uloz dump pre debug */
    FILE *fdump = fopen("keys/app_openssl_pem_dump.txt", "w");
    if (fdump) {
        fwrite(text, 1, text_size, fdump);
        fclose(fdump);
    }

    printf("=== PEM -> seed + kluce pre aplikaciu (ML-DSA-44) ===\n");
    printf("Vstupny PEM: %s\n\n", pem_path);

    /* 2) SEED */
    char *seed_hex = extract_block(text, "seed:");
    if (!seed_hex) {
        fprintf(stderr, "Upozornenie: seed blok (seed:) sa v textovom vystupe nenasiel.\n");
    } else {
        size_t seed_hex_len = strlen(seed_hex);
        if (seed_hex_len != SEEDBYTES * 2) {
            printf("Upozornenie: dlzka seed hex = %zu, ocakava sa %d\n",
                   seed_hex_len, SEEDBYTES * 2);
        }

        uint8_t seed_bin[SEEDBYTES] = {0};
        if (hex_to_bin(seed_hex, seed_bin, sizeof(seed_bin)) < 0) {
            fprintf(stderr, "Chyba dekodovania seed hex.\n");
            free(seed_hex);
            free(text);
            return 1;
        }

        printf("Nacitany 32-bajtovy seed (hex):\n%s\n\n", seed_hex);

        save_bin("keys/app_openssl_seed.bin", seed_bin, SEEDBYTES);
        FILE *fseedtxt = fopen("keys/app_openssl_seed.hex", "w");
        if (fseedtxt) {
            fprintf(fseedtxt, "%s\n", seed_hex);
            fclose(fseedtxt);
            printf("  -> ulozene keys/app_openssl_seed.hex\n");
        } else {
            printf("Upozornenie: nepodarilo sa zapisat keys/app_openssl_seed.hex\n");
        }

        free(seed_hex);
    }

    /* 3) PRIV (secret key v PQClean formate) */
    char *priv_hex = extract_block(text, "priv:");
    uint8_t *priv_bin = NULL;
    size_t priv_len = 0;

    if (!priv_hex) {
        fprintf(stderr, "Upozornenie: blok priv: sa nenasiel.\n");
    } else {
        size_t hexlen = strlen(priv_hex);
        priv_len = hexlen / 2;
        priv_bin = malloc(priv_len);
        if (!priv_bin) {
            fprintf(stderr, "Nedostatok pamate pre priv_bin\n");
            free(priv_hex);
            free(text);
            return 1;
        }
        if (hex_to_bin(priv_hex, priv_bin, priv_len) < 0) {
            fprintf(stderr, "Chyba dekodovania priv hex.\n");
            free(priv_hex);
            free(priv_bin);
            free(text);
            return 1;
        }
        free(priv_hex);

        printf("\nDlzka priv bloku z OpenSSL: %zu bajtov\n", priv_len);

        if (priv_len < PQCLEAN_MLDSA44_CLEAN_CRYPTO_SECRETKEYBYTES) {
            printf("Upozornenie: priv blok je kratsi ako %d bajtov.\n",
                   PQCLEAN_MLDSA44_CLEAN_CRYPTO_SECRETKEYBYTES);
        } else {
            save_bin("keys/app_openssl_sk.bin",
                     priv_bin,
                     PQCLEAN_MLDSA44_CLEAN_CRYPTO_SECRETKEYBYTES);
        }
    }

    /* 4) PUB (public key pre tvoju appku) */
    char *pub_hex = extract_block(text, "pub:");
    if (!pub_hex) {
        fprintf(stderr, "Upozornenie: blok pub: sa nenasiel.\n");
    } else {
        size_t pub_len = strlen(pub_hex) / 2;
        uint8_t *pub_bin = malloc(pub_len);
        if (!pub_bin) {
            fprintf(stderr, "Nedostatok pamate pre pub_bin\n");
            free(pub_hex);
            free(priv_bin);
            free(text);
            return 1;
        }
        if (hex_to_bin(pub_hex, pub_bin, pub_len) < 0) {
            fprintf(stderr, "Chyba dekodovania pub hex.\n");
            free(pub_hex);
            free(pub_bin);
            free(priv_bin);
            free(text);
            return 1;
        }
        free(pub_hex);

        printf("\nDlzka pub bloku z OpenSSL: %zu bajtov\n", pub_len);

        if (pub_len != PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES) {
            printf("Upozornenie: public key ma dlzku %zu, ocakava sa %d\n",
                   pub_len, PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES);
        }

        save_bin("keys/app_openssl_pk.bin",
                 pub_bin,
                 pub_len < PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES
                     ? pub_len
                     : PQCLEAN_MLDSA44_CLEAN_CRYPTO_PUBLICKEYBYTES);

        free(pub_bin);
    }

    free(priv_bin);
    free(text);

    printf("\nHotovo. app_openssl_* subory su ulozene v priecinku keys/.\n");
    return 0;
}
