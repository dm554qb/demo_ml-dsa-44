
# demo_ml-dsa-44  
Implementácia a testovacia sada pre ML-DSA-44 (post-kvantová podpisová schéma, FIPS‑204)

Tento projekt obsahuje kompletnú implementáciu ML‑DSA‑44 založenú na PQClean, doplnenú o
vlastné nástroje na generovanie kľúčov, podpisovanie, verifikáciu a obojsmernú
kompatibilitu s OpenSSL (PEM ↔ raw .bin formáty).

---

## 📁 Štruktúra projektu

```
source/
    genkey.c               – generovanie kľúčov + deterministický seed
    sign_file.c            – podpisovanie súboru pomocou secretkey.bin
    verify.c               – overenie podpisu
    openssl_from_app.c     – prevod APP → OpenSSL (seed → PEM → raw bloky)
    app_from_openssl.c     – prevod OpenSSL → APP (PEM → seed/pk/sk .bin)
    api.h, params.h        – parametre ML-DSA-44
    poly*, ntt*, reduce*, rounding*, packing*, sign.c – jadro PQClean

common/
    fips202, shake, sha2, randombytes, keccak, sp800-185…

keys/
    – sem sa ukladajú kľúče, seed, dumpy a konverzie

files/
    test_text*.txt, test_bin.bin – ukážkové dáta
```

---

## 🔐 Funkcionalita

### 1. Generovanie kľúčov  
```
./genkey
```
Výsledok:
- `keys/app_publickey.bin` (1312 B)  
- `keys/app_secretkey.bin` (2560 B)  
- `keys/app_seed.bin` (32 B raw)  
- `keys/app_seed.hex` (64 hex znakov – kompatibilné s OpenSSL)

### 2. Podpisovanie  
```
./sign_file <subor> <secretkey.bin>
```
Výstup:  
- `signature.bin` (2420 B)

### 3. Overenie podpisu  
```
./verify <subor> <publickey.bin> <signature.bin>
```

### 4. Export APP → OpenSSL  
```
./openssl_from_app
```
Použije seed z APP a vygeneruje:  
- `keys/openssl_app_key.pem`  
- `keys/openssl_appkey_dump.txt`  
- `keys/openssl_app_seed.bin`  
- `keys/openssl_app_sk.bin`  
- `keys/openssl_app_pk.bin`

### 5. Import OpenSSL → APP  
```
./app_from_openssl <pem_súbor>
```
Parsuje PEM a uloží:  
- `keys/app_openssl_seed.bin`  
- `keys/app_openssl_sk.bin`  
- `keys/app_openssl_pk.bin`

---

## 🧪 Test kompatibility

1. `./genkey`  
2. `./openssl_from_app`  
3. porovnať:

```
diff keys/app_publickey.bin keys/openssl_app_pk.bin
diff keys/app_secretkey.bin keys/openssl_app_sk.bin   (len ak zodpovedá layout)
```

4. `./app_from_openssl keys/openssl_app_key.pem`  
5. opäť porovnať:

```
diff keys/app_openssl_pk.bin keys/app_publickey.bin
```

---

## 🛠️ Kompilácia

Projekt používa GCC a OpenSSL 3.5+.  
Jednoduchý build:

```
make
```

Možné targety:  
- `genkey`  
- `sign_file`  
- `verify`  
- `openssl_from_app`  
- `app_from_openssl`

---

## 📌 Poznámky

- Implementácia ML‑DSA‑44 je prevzatá z PQClean (korektná, bezpečná, bez úprav algoritmu).  
- Nástroje pre prácu s PEM sú zámerne low-level (popen → text dump → hex parsing).  
- Kompatibilita závisí od formátu `openssl pkey -text` – môže sa meniť medzi verziami.  
- Projekt je určený pre vzdelávacie a výskumné účely (diplomová práca).

---

## © Autor
Dávid Mudrák  
TUKE – Dipl. práca „Post‑kvantové digitálne podpisy“  
2025
