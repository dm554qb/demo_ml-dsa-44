# ===== ML-DSA-44 PROJECT =====
CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Isource -Icommon -IC:/OPENSSL/include

# Na Windows/MinGW nelinkujeme priamo libcrypto/libssl,
# lebo pouzivame len externy program openssl.exe, nie C API.
LDFLAGS =

SRC_DIR = source
COMMON_DIR = common
KEYS_DIR = keys

# Spolocne zdrojaky (okrem hlavnych .c)
COMMON_SRC = $(wildcard $(COMMON_DIR)/*.c)

# Vylucime hlavne programy, aby neboli dvakrat prelozene
SRC_BASE = $(filter-out \
	$(SRC_DIR)/genkey.c \
	$(SRC_DIR)/sign_file.c \
	$(SRC_DIR)/verify.c \
	$(SRC_DIR)/openssl_from_app.c \
	$(SRC_DIR)/app_from_openssl.c, \
	$(wildcard $(SRC_DIR)/*.c))

# Binarky ML-DSA
BINARIES = genkey sign_file verify openssl_from_app app_from_openssl

# ===== All =====
all: $(BINARIES)

# ----- ML-DSA executables -----
genkey: $(SRC_DIR)/genkey.c $(SRC_BASE) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sign_file: $(SRC_DIR)/sign_file.c $(SRC_BASE) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

verify: $(SRC_DIR)/verify.c $(SRC_BASE) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ----- OpenSSL direction: APP -> OpenSSL -----
openssl_from_app: $(SRC_DIR)/openssl_from_app.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ----- OpenSSL direction: OpenSSL -> APP -----
app_from_openssl: $(SRC_DIR)/app_from_openssl.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ----- Utility targets -----
clean:
	rm -f $(SRC_DIR)/*.o $(COMMON_DIR)/*.o $(BINARIES)
	rm -f *.exe $(SRC_DIR)/*.exe $(COMMON_DIR)/*.exe
	rm -f $(KEYS_DIR)/*.pem $(KEYS_DIR)/*.bin $(KEYS_DIR)/*.txt $(KEYS_DIR)/*.hex $(KEYS_DIR)/*.sig

clean_keys:
	rm -f $(KEYS_DIR)/*.pem $(KEYS_DIR)/*.bin $(KEYS_DIR)/*.txt $(KEYS_DIR)/*.hex $(KEYS_DIR)/*.sig

.PHONY: all clean
