# ML-DSA-44 Demo Application  
Post-Quantum Digital Signature Demo (PQClean + OpenSSL)

Tento projekt demonštruje kompletnú implementáciu post-kvantovej podpisovej schémy **ML-DSA-44 (FIPS-204)** na praktické generovanie kľúčov, podpisovanie a overovanie súborov.

Implementácia je založená na **PQClean ML-DSA-44 (Dilithium-2)** a doplnená o:
- generovanie kľúčov kompatibilných s OpenSSL pomocou 32-bajtového seedu,
- konverziu OpenSSL PEM → binárnych PQClean formátov,
- porovnávanie kľúčov (OpenSSL vs. aplikácia),
- podpis a overovanie ľubovoľných súborov.

Projekt je navrhnutý ako praktická demonštrácia k diplomovej práci  
**„Post-kvantové digitálne podpisy“ (TUKE FEI, 2025)**.

---

## 📁 Štruktúra projektu

```
source/                   // implementácia ML-DSA-44 (PQClean)
    genkey.c
    sign_file.c
    verify.c
    openssl_from_seed.c
    *.c *.h (ntt, poly, packing, rounding, reduce, fips202 …)
common/                   // SHAKE, randombytes, AES
keys/                     // generované kľúče a podpisy
files/                    // testovacie vstupy

Makefile
README.md
```

---

## 🔧 Kompilácia

Stačí spustiť:

```
make
```

Vytvoria sa binárky:

- `genkey`
- `sign_file`
- `verify`
- `openssl_from_seed`

---

# 🔑 1. Generovanie kľúčov (app → PQClean)

```
./genkey
```

Program urobí:

1. vygeneruje **32-bajtový seed**,
2. odvodením (shake256) získa: rho, rhoprime, key,
3. vygeneruje ML-DSA-44 kľúče podľa PQClean,
4. uloží:

| Súbor | Popis |
|------|-------|
| `keys/app_publickey.bin` | 1312 bajtov |
| `keys/app_secretkey.bin` | 2560 bajtov |
| `keys/app_seed.bin` | raw 32 bajtov |
| `keys/app_seed.hex` | hex formát pre OpenSSL |

Program vypíše aj príkazy:

```
openssl genpkey -algorithm ML-DSA-44 -pkeyopt seed:<seedhex> -out openssl_private.pem
openssl pkey -in openssl_private.pem -pubout -out openssl_public.pem
```

---

# ✍️ 2. Podpisovanie súboru

```
./sign_file <subor> <sukromny_kluc.bin>
```

Výstup:  
**signature.bin** – ML-DSA-44 podpis (2420 bajtov)

---

# 🔍 3. Overovanie podpisu

```
./verify <subor> <verejny_kluc.bin> <signature.bin>
```

---

# 🔄 4. OpenSSL kľúče zo seedu

```
./openssl_from_seed
```

Vytvorí:

- `keys/openssl_key.pem`
- `keys/openssl_seed.bin`
- `keys/openssl_secretkey.bin`
- `keys/openssl_publickey.bin`
- `keys/openssl_key_dump.txt`

---

# 🧪 Testovací scenár

```
./genkey
./sign_file files/test_bin.bin keys/app_secretkey.bin
./verify files/test_bin.bin keys/app_publickey.bin signature.bin
./openssl_from_seed
```

---

# 🎓 Autor

**Dávid Mudrák**, TUKE FEI, 2025  
Téma: *Post-kvantové digitálne podpisy*

---

# 📜 Licencia

Časť implementácie z projektu **PQClean** – MIT/CC0.  
Ostatné súbory – MIT.

