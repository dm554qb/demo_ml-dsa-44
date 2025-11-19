# 🔐 ML-DSA-44 – Generovanie, export a overovanie kľúčov a podpisov medzi OpenSSL a mojou aplikáciou

Tento projekt demonštruje **plne funkčnú implementáciu post-kvantového podpisového algoritmu ML-DSA-44 (FIPS 204)** s prepojením na **OpenSSL 3.5+**.  
Cieľom je ukázať kompatibilitu medzi implementáciou z knižnice **PQClean** a OpenSSL — generovanie, podpisovanie, overovanie a *obojsmerný export/import kľúčov aj podpisov*.

---

## 🧩 Použité prostredie

Testované v:

- **OpenSSL 3.5.2** / **OpenSSL 3.6.0**
- **Ubuntu 24.04 LTS** + obraz **BIKS**
- Kompilácia: `make`

---

# ⚙️ Prehľad implementovaných nástrojov

---

## 🧠 1. `genkey` – Generovanie APP kľúčov (PQClean)

```bash
./genkey
# Generuje ML-DSA-44 kľúčový pár pomocou PQClean a vytvorí 32B deterministický seed.
```

Výsledok:

- `keys/app_sk.bin` – súkromný kľúč (2560 B)  
- `keys/app_pk.bin` – verejný kľúč (1312 B)  
- `keys/app_seed.bin` – seed (32 B)  
- `keys/app_seed.hex` – seed (64 hex znakov)  

➡️ Tento seed umožňuje **v OpenSSL vygenerovať identické kľúče**.

---

## ✍️ 2. `sign_file` – Podpis pomocou APP (PQClean)

```bash
./sign_file <subor> <sukromny_kluc>
# Podpíše binárny súbor pomocou ML-DSA-44 APP implementácie.
```

Príklad:

```bash
./sign_file files/test_bin.bin keys/app_sk.bin
# Výstup: keys/app_sign.bin
```

---

## 🔍 3. `verify` – Overenie podpisu pomocou APP

```bash
./verify <subor> <verejny_kluc> <podpis>
# Overí ML-DSA-44 podpis pomocou aplikácie.
```

---

# 🔐 4. `openssl_from_app` – Export APP → OpenSSL (zo seedu)

```bash
./openssl_from_app keys/app_seed.bin
# Načíta 32B seed → OpenSSL pomocou neho vygeneruje identický ML-DSA-44 kľúč.
```

Výstupy:

- `keys/openssl_app_key.pem`
- `keys/openssl_app_pk.pem`
- `keys/openssl_app_seed.bin`
- `keys/openssl_app_sk.bin`
- `keys/openssl_app_pk.bin`

---

# 🔄 5. `app_from_openssl` – Import OpenSSL → APP

```bash
./app_from_openssl keys/openssl_key.pem
# Extrahuje seed + public/secret key z OpenSSL PEM do PQClean formátu.
```

Výstupy:

- `keys/app_openssl_seed.bin`
- `keys/app_openssl_sk.bin`
- `keys/app_openssl_pk.bin`

---

# 🧰 OpenSSL príkazy

---

## 🔹 Generovanie OpenSSL kľúčov zo seedu

```bash
openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed_hex> -out keys/openssl_app_key.pem
# Deterministická tvorba OpenSSL ML-DSA-44 kľúča zo seedu.
```

---

## 🔹 Dump OpenSSL kľúča

```bash
openssl pkey -in keys/openssl_app_key.pem -text -noout > keys/openssl_appkey_dump.txt
# Zobrazuje seed, public key a secret key v textovej/hex forme.
```

---

## 🔹 Export kľúčov z OpenSSL

```bash
openssl pkey -in keys/openssl_app_key.pem -out keys/openssl_app_sk.pem
# Extrahuje súkromný kľúč v PEM formáte.
```

```bash
openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem
# Extrahuje verejný kľúč v PEM formáte.
```

---

# ✉️ Podpisovanie pomocou OpenSSL

```bash
openssl pkeyutl -sign   -inkey keys/openssl_app_sk.pem   -rawin   -in files/test_bin.bin   -out keys/openssl_app_sign.sig
# Podpíše súbor pomocou ML-DSA-44 v OpenSSL.
```

---

# ✉️ Overenie pomocou OpenSSL

```bash
openssl pkeyutl -verify -pubin   -inkey keys/openssl_app_pk.pem   -rawin   -in files/test_bin.bin   -sigfile keys/openssl_app_sign.sig
# Overí ML-DSA-44 podpis cez OpenSSL.
```

---

# 🔄 Obojsmerné kombinácie (APP ↔ OPENSSL)

APP → OpenSSL:

```bash
./sign_file files/test_bin.bin keys/app_sk.bin
# Podpis vytvorený aplikáciou.
openssl pkeyutl -verify -pubin -inkey keys/openssl_app_pk.pem -rawin   -in files/test_bin.bin -sigfile keys/app_sign.bin
# Overenie cez OpenSSL.
```

OPENSSL → APP:

```bash
openssl pkeyutl -sign -inkey keys/openssl_app_sk.pem -rawin   -in files/test_bin.bin -out keys/openssl_app_sign.sig
# Podpis v OpenSSL.

./verify files/test_bin.bin keys/app_pk.bin keys/openssl_app_sign.sig
# Overenie cez aplikáciu.
```

---

# 🔍 Porovnávanie výstupov (identickosť kľúčov/podpisov)

```bash
cmp keys/app_pk.bin keys/openssl_app_pk.bin
# Identické public keys.
```

```bash
cmp keys/app_sk.bin keys/openssl_app_sk.bin
# Identické secret keys.
```

```bash
cmp keys/app_sign.bin keys/openssl_app_sign.sig
# Identické podpisy (pri rovnakom seede a message).
```

```bash
cmp keys/app_seed.bin keys/openssl_app_seed.bin
# Identický seed = identické kľúče.
```

---

# 🚀 Kompatibilita

✔ PQClean → OpenSSL  
✔ OpenSSL → PQClean  
✔ Identické kľúče pri rovnakom seede  
✔ Podpisy sú zameniteľné  
✔ Obojsmerné overenie funguje

---

# 👤 Autor

**Dávid Mudrák**  
Diplomová práca: *Post-kvantové digitálne podpisy (ML-DSA-44)*  
TUKE – FEI, Počítačové siete
