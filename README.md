# 🔐 ML-DSA-44 – Generovanie, export a overovanie kľúčov a podpisov medzi OpenSSL a mojou aplikáciou

Tento projekt demonštruje **plne funkčnú implementáciu post-kvantového podpisového algoritmu ML-DSA-44 (FIPS 204)** s prepojením na **OpenSSL 3.5+**.
Cieľom je ukázať kompatibilitu medzi implementáciou z knižnice **PQClean** a OpenSSL — generovanie, podpisovanie, overovanie a obojsmerný export/import kľúčov.

---

## 🧩 Použité prostredie

Testované v:

- **OpenSSL 3.5.2** / **OpenSSL 3.6.0**
- **Ubuntu 24.04 LTS** a obraz **BIKS**
- Kompilácia cez `make`

---

## ⚙️ Prehľad implementovaných nástrojov

### 🧠 `genkey` – Generovanie kľúčov

Generuje ML-DSA-44 kľúče a deterministický seed:

- `app_sk.bin` – súkromný kľúč  
- `app_pk.bin` – verejný kľúč  
- `app_seed.bin` – raw 32-bajtový seed  
- `app_seed.hex` – 64 ASCII hex seed  

Seed potom umožňuje **získať totožné kľúče v OpenSSL**.

---

### ✍️ `sign_file`
```
./sign_file <subor_na_podpisanie> <subor_so_sukromnym_klucom>
```
Výstup: `signature.bin`

---

### 🔍 `verify`
```
./verify <subor_na_overenie> <publickey.bin> <signature.bin>
```
Overí podpis pomocou ML-DSA-44 public key.

---

### 🔐 `openssl_from_app` – Export APP → OpenSSL (NOVÉ)

```
./openssl_from_app <seed.bin | seed.hex>
```

Podporované formáty:

- `seed.bin` → 32 bajtov  
- `seed.hex` → 64 hex znakov (bez medzier / s medzerami / s dvojbodkami)

Program spraví:

1. Načíta seed  
2. Spustí deterministické generovanie OpenSSL kľúča:
   ```
   openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed>
   ```
3. Extrahuje z OpenSSL:

- `keys/openssl_app_key.pem`  
- `keys/openssl_app_pk.pem`  
- `keys/openssl_appkey_dump.txt`  
- `keys/openssl_app_seed.bin`  
- `keys/openssl_app_sk.bin`  
- `keys/openssl_app_pk.bin`  

---

### 🔄 `app_from_openssl` – Import OpenSSL → APP (NOVÉ)

```
./app_from_openssl <openssl_pem_subor>
```

Extrahuje z PEM → APP formátu:

- `keys/app_openssl_seed.bin`  
- `keys/app_openssl_sk.bin`  
- `keys/app_openssl_pk.bin`  

---

## 🧰 OpenSSL príkazy

### Generovanie zo seedu
```
openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed_hex> -out keys/openssl_app_key.pem
```

### Dump
```
openssl pkey -in keys/openssl_app_key.pem -text -noout > keys/openssl_appkey_dump.txt
```

### Export
```
openssl pkey -in keys/openssl_app_key.pem -out keys/openssl_app_sk.pem
openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem
```

### Extrakcia raw blokov
```
grep -A 3 "^seed" keys/openssl_appkey_dump.txt | grep -v "seed" | tr -d ' 
:' | xxd -r -p > keys/openssl_seed.bin
awk '/^priv:/ {flag=1; next} /^pub:/ {flag=0} flag' keys/openssl_appkey_dump.txt | tr -d ' 
:' | xxd -r -p > keys/openssl_secretkey.bin
grep -A 999 "^pub" keys/openssl_appkey_dump.txt | grep -v "pub" | tr -d ' 
:' | xxd -r -p > keys/openssl_publickey.bin
```

---

## ✉️ Podpisovanie / overovanie cez OpenSSL

### Podpis
```
openssl pkeyutl -sign -inkey keys/openssl_app_sk.pem -in files/test_bin.bin -out keys/openssl_app_sign.sig
```

### Overenie
```
openssl pkeyutl -verify -pubin -inkey keys/openssl_app_pk.pem -in files/test_bin.bin -sigfile keys/openssl_app_sign.sig
```

---

## 📁 Štruktúra projektu

```
demo_ml-dsa-44/
├── source/
│   ├── genkey.c
│   ├── sign_file.c
│   ├── verify.c
│   ├── openssl_from_app.c
│   └── app_from_openssl.c
├── common/
├── keys/
├── files/
├── Makefile
└── README.md
```

---

## 🚀 Príklady

### Generovanie APP kľúčov
```
./genkey
```

### Podpis
```
./sign_file files/test_bin.bin keys/app_sk.bin
```

### Overenie
```
./verify files/test_bin.bin keys/app_pk.bin signature.bin
```

### Identické OpenSSL kľúče
```
./openssl_from_app keys/app_seed.bin
```

### Import PEM → APP
```
./app_from_openssl keys/openssl_app_key.pem
```

---

## 🧾 Poznámky

- Testované na **OpenSSL 3.5.2 / 3.6.0**
- Implementácia rešpektuje **FIPS 204**
- APP ↔ OpenSSL kompatibilita: kľúče aj podpisy sú zameniteľné
- Export/import funguje obojsmerne

---

## 👤 Autor
**Dávid Mudrák**  
Diplomová práca: *Post-kvantové digitálne podpisy (ML-DSA-44)*  
TUKE – FEI, Počítačové siete
