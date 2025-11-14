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

#define SEEDBYTES 32

static void ensure_keys_dir(void) {
#ifdef _WIN32
    _mkdir("keys"); // existujuci nevadi
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
    if (len % 2 != 0) return -1;
    for (size_t i = 0; i < len / 2 && i < outlen; i++) {
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

static int run_to_file(const char *cmd, const char *outfile) {
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "Nepodarilo sa spustit: %s\n", cmd);
        return -1;
    }
    FILE *out = fopen(outfile, "wb");
    if (!out) {
        fprintf(stderr, "Nepodarilo sa otvorit %s na zapis\n", outfile);
        pclose(pipe);
        return -1;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0)
        fwrite(buf, 1, n, out);

    fclose(out);
    int rc = pclose(pipe);
    if (rc != 0) {
        fprintf(stderr, "OpenSSL vratil kod %d pri: %s\n", rc, cmd);
        return -1;
    }
    printf("Vytvoreny subor: %s\n", outfile);
    return 0;
}

static int run_status(const char *cmd) {
    int rc = system(cmd);
#ifdef _WIN32
    if (rc == -1) {
        fprintf(stderr, "Zlyhalo spustenie prikazu (system): %s\n", cmd);
        return -1;
    }
#else
    if (rc == -1 || WEXITSTATUS(rc) != 0) {
        fprintf(stderr, "Prikaz skoncil chybou: %s (rc=%d)\n", cmd, rc);
        return -1;
    }
#endif
    return 0;
}

int main(void) {
    ensure_keys_dir();

    char seed_hex[2 * SEEDBYTES + 1] = {0};
    uint8_t seed_bin[SEEDBYTES];
    FILE *f;
    int have_seed = 0;

    printf("=== OpenSSL from Seed (ML-DSA-44) ===\n");

    // 0) Nacitaj seed (preferuj bin, potom hex)
    if ((f = fopen("keys/app_seed.bin", "rb")) != NULL) {
        if (fread(seed_bin, 1, SEEDBYTES, f) == SEEDBYTES) {
            for (int i = 0; i < SEEDBYTES; i++)
                sprintf(seed_hex + 2 * i, "%02x", seed_bin[i]);
            have_seed = 1;
            printf("Nacitany seed z keys/app_seed.bin\n");
        }
        fclose(f);
    } else if ((f = fopen("keys/app_seed.hex", "r")) != NULL) {
        if (fgets(seed_hex, sizeof(seed_hex), f)) {
            for (int i = 0; seed_hex[i]; i++)
                if (isspace((unsigned char)seed_hex[i]))
                    seed_hex[i] = '\0';
            have_seed = 1;
            printf("Nacitany seed z keys/app_seed.hex\n");
        }
        fclose(f);
    }

    if (!have_seed) {
        fprintf(stderr, "Seed sa nenasiel (chyba keys/app_seed.bin alebo keys/app_seed.hex)\n");
        return 1;
    }

    // 1) openssl genpkey z hexa seedu -> PEM (cez popen -> subor)
    char cmd[700];
    snprintf(cmd, sizeof(cmd),
             "openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:%s",
             seed_hex);

    if (run_to_file(cmd, "keys/openssl_app_key.pem") != 0)
        return 1;

    // 2) Dump textu (seed/priv/pub hexy) do txt
    if (run_to_file("openssl pkey -in keys/openssl_app_key.pem -text -noout",
                    "keys/openssl_appkey_dump.txt") != 0) {
        fprintf(stderr, "Nepodarilo sa vytvorit textovy dump.\n");
    }

    // 3) Export verejneho kluca do PEM
    if (run_status("openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem") != 0)
        return 1;

    // 4) Z dumpu vytiahni seed/priv/pub do .bin
    FILE *fd = fopen("keys/openssl_appkey_dump.txt", "rb");
    if (fd) {
        fseek(fd, 0, SEEK_END);
        long tlen = ftell(fd);
        rewind(fd);
        char *text = (char*)malloc((size_t)tlen + 1);
        if (text && fread(text, 1, (size_t)tlen, fd) == (size_t)tlen) {
            text[tlen] = '\0';

            const char *names[] = {"seed:", "priv:", "pub:"};
            const char *out_files[] = {
                "keys/openssl_app_seed.bin",
                "keys/openssl_app_sk.bin",
                "keys/openssl_app_pk.bin"
            };

            for (int nidx = 0; nidx < 3; nidx++) {
                const char *start = strstr(text, names[nidx]);
                if (!start) {
                    printf("Nenajdeny blok %s v dump subore\n", names[nidx]);
                    continue;
                }
                const char *p = strchr(start, '\n');
                if (!p) continue;
                p++;

                char block[65536] = {0};
                size_t bi = 0;

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
                    printf("Blok %s je prazdny\n", names[nidx]);
                    continue;
                }

                uint8_t *out = (uint8_t*)malloc(outlen);
                if (!out) {
                    fprintf(stderr, "Nedostatok pamate pri %s\n", names[nidx]);
                    continue;
                }

                if (hex_to_bin(block, out, outlen) > 0)
                    save_bin(out_files[nidx], out, outlen);
                else
                    printf("Chyba dekodovania %s\n", names[nidx]);

                free(out);
            }
        } else {
            fprintf(stderr, "Nepodarilo sa precitat keys/openssl_appkey_dump.txt\n");
        }
        if (text) free(text);
        fclose(fd);
    }

    printf("\nHotovo. Vystupy v priecinku keys/:\n");
    printf("   - openssl_app_key.pem\n");
    printf("   - openssl_app_pk.pem\n");
    printf("   - openssl_appkey_dump.txt\n");
    printf("   - openssl_app_seed.bin / openssl_app_sk.bin / openssl_app_pk.bin\n");

    return 0;
}
