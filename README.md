# 🔐 ML-DSA-44 – Generovanie, export a overovanie kľúčov a podpisov medzi OpenSSL a mojou aplikáciou

Tento projekt demonštruje **plne funkčnú implementáciu post-kvantového podpisového algoritmu ML-DSA-44 (FIPS 204)** s prepojením na **OpenSSL 3.5+**.  
Cieľom je ukázať kompatibilitu medzi implementáciou z knižnice **PQClean** a nástrojmi OpenSSL – generovanie, podpisovanie, overovanie a export kľúčov.

---

## 🧩 Použité prostredie

Všetky príkazy a binárky boli testované v prostrediach:
- **OpenSSL 3.5.2** a **OpenSSL 3.6.0**
- **Ubuntu 24.04 LTS** a obraz **BIKS**
- Kompatibilné aj s Windows (MinGW / VSCode / XAMPP)

---

## ⚙️ Prehľad implementovaných nástrojov

### 🧠 `genkey`
Generuje ML-DSA-44 kľúče:
- `app_sk.bin` – súkromný kľúč  
- `app_pk.bin` – verejný kľúč  
- `app_seed.bin` – 32-bajtový raw binárny seed  
- `app_seed.hex` – ASCII reprezentácia seedu  

Súbor `.bin` sa používa pri generovaní rovnakého kľúčového páru v OpenSSL.

---

### ✍️ `sign_file`
```bash
sign_file <subor_na_podpisanie> <subor_so_sukromnym_klucom>
```
Vytvorí podpis súboru a uloží ho ako `signature.bin`.

---

### 🔍 `verify`
```bash
verify <subor_na_overenie> <publickey.bin> <signature.bin>
```
Overí platnosť podpisu voči zadanému súboru a verejnému kľúču.

---

### 🔐 `openssl_from_seed`
Načíta `app_seed.bin` (ak existuje) a prevedie ho na hex.  
Následne spustí:
```bash
openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed>
```
aby OpenSSL vygeneroval **presne ten istý pár kľúčov**.  
OpenSSL potom vykoná:
```bash
openssl pkey -in keys/openssl_key.pem -text -noout
```
a z výstupu extrahuje sekcie `seed:`, `priv:` a `pub:`, ktoré sú prevedené do binárnej podoby:

- `keys/openssl_app_seed.bin` – binárny obsah seedu  
- `keys/openssl_app_sk.bin` – privátny kľúč z OpenSSL  
- `keys/openssl_app_pk.bin` – verejný kľúč z OpenSSL  

---

## 🧰 Samostatné príkazy pre Linux konzolu

### 🔧 Generovanie kľúčov
```bash
openssl genpkey -algorithm ML-DSA-44 -out keys/openssl_app_key.pem -pkeyopt hexseed:<seed>
openssl pkey -in keys/openssl_app_key.pem -text -noout > keys/openssl_appkey_dump.txt
openssl pkey -in keys/openssl_app_key.pem -out keys/openssl_app_sk.pem
openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem
```

### 📦 Extrakcia raw `.bin` dát pre porovnanie binárnych kľúčov
```bash
grep -A 3 "^seed" keys/openssl_appkey_dump.txt | grep -v "seed" | tr -d ' \n:' | xxd -r -p > keys/openssl_seed.bin
awk '/^priv:/ {flag=1; next} /^pub:/ {flag=0} flag' keys/openssl_appkey_dump.txt | tr -d ' \n:' | xxd -r -p > keys/openssl_secretkey.bin
grep -A 999 "^pub" keys/openssl_appkey_dump.txt | grep -v "pub" | tr -d ' \n:' | xxd -r -p > keys/openssl_publickey.bin
```

---

## ✉️ Podpisovanie a overovanie pomocou OpenSSL

### 🖋️ Podpis súboru
```bash
openssl pkeyutl -sign -inkey keys/openssl_app_sk.pem -in files/test_bin.bin -out keys/openssl_app_sign.sig
```

### ✅ Overenie podpisu
```bash
openssl pkeyutl -verify -pubin -inkey keys/openssl_app_pk.pem -in files/test_bin.bin -sigfile keys/openssl_app_sign.sig
```

---

## 📁 Štruktúra priečinkov

```
demo_ml-dsa-44/
│
├── source/                # Zdrojové kódy (PQClean + vlastné)
│   ├── genkey.c
│   ├── sign_file.c
│   ├── verify.c
│   └── openssl_from_seed.c
│
├── common/                # Pomocné knižnice (fips202, randombytes, …)
│
├── keys/                  # Generované kľúče
│   ├── app_pk.bin
│   ├── app_sk.bin
│   ├── app_seed.bin
│   ├── openssl_app_pk.pem
│   ├── openssl_app_sk.pem
│   └── openssl_app_key.pem
│
├── files/                 # Testovacie vstupné súbory
│   └── test_bin.bin
│
├── Makefile               # Kompilácia všetkých utilít
└── README.md              # Tento dokument
```

---

## 🚀 Príklady spustenia

### 1️⃣ Generovanie kľúčov
```bash
./genkey
```

### 2️⃣ Podpis súboru
```bash
./sign_file files/test_bin.bin keys/app_sk.bin
```

### 3️⃣ Overenie podpisu
```bash
./verify files/test_bin.bin keys/app_pk.bin signature.bin
```

### 4️⃣ Generovanie OpenSSL kľúčov zo seedu
```bash
./openssl_from_seed
```

---

## 🧾 Poznámky

- Všetky príkazy boli overené na **OpenSSL 3.5.2 / 3.6.0**  
- Implementácia je plne kompatibilná s algoritmom **ML-DSA-44 (FIPS 204)**  
- Vygenerované podpisy a kľúče sú medzi aplikáciou a OpenSSL **vzájomne overiteľné**

---

## 👤 Autor
**Dávid Mudrák**  
Projekt diplomovej práce: *Post-kvantové digitálne podpisy (ML-DSA-44)*  
Technická univerzita v Košiciach, FEI – odbor Počítačové siete
