# ===== ML-DSA-44 PROJECT =====
CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Isource -Icommon -IC:/OPENSSL/include
#LDFLAGS = -LC:/OPENSSL/lib -lcrypto -lssl

SRC_DIR = source
COMMON_DIR = common
KEYS_DIR = keys

# Spoločné zdrojáky (bez hlavných .c súborov)
COMMON_SRC = $(wildcard $(COMMON_DIR)/*.c)
SRC_BASE = $(filter-out $(SRC_DIR)/genkey.c $(SRC_DIR)/sign_file.c $(SRC_DIR)/verify.c $(SRC_DIR)/openssl_from_seed.c, $(wildcard $(SRC_DIR)/*.c))

# Binárky ML-DSA
BINARIES = genkey sign_file verify openssl_from_seed

# ===== All =====
all: $(BINARIES)

# ----- ML-DSA executables -----
genkey: $(SRC_DIR)/genkey.c $(SRC_BASE) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sign_file: $(SRC_DIR)/sign_file.c $(SRC_BASE) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

verify: $(SRC_DIR)/verify.c $(SRC_BASE) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

openssl_from_seed: $(SRC_DIR)/openssl_from_seed.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ----- Utility targets -----
clean:
	rm -f $(SRC_DIR)/*.o $(COMMON_DIR)/*.o $(BINARIES)
	rm -f *.exe $(SRC_DIR)/*.exe $(COMMON_DIR)/*.exe $(KEYS_DIR)/*.pem $(KEYS_DIR)/*.bin $(KEYS_DIR)/*.txt $(KEYS_DIR)/*.hex $(KEYS_DIR)/*.sig

.PHONY: all clean
