# 🔐 ML-DSA-44 – Generovanie, export a overovanie kľúčov a podpisov medzi OpenSSL a mojou aplikáciou

Tento projekt demonštruje **plne funkčnú implementáciu post-kvantového podpisového algoritmu ML-DSA-44 (FIPS 204)** s prepojením na **OpenSSL 3.5+**.
Cieľom je ukázať kompatibilitu medzi implementáciou z knižnice **PQClean** a OpenSSL — generovanie, podpisovanie, overovanie a obojsmerný export/import kľúčov a podpisov.

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

- `keys/app_sk.bin` – súkromný kľúč (2560 B)  
- `keys/app_pk.bin` – verejný kľúč (1312 B)  
- `keys/app_seed.bin` – raw 32-bajtový seed  
- `keys/app_seed.hex` – 64 ASCII hex seed  

Seed potom umožňuje **získať totožné kľúče v OpenSSL**.

---

### ✍️ `sign_file`

```bash
./sign_file <subor_na_podpisanie> <subor_so_sukromnym_klucom>
# Príklad:
./sign_file files/test_bin.bin keys/app_sk.bin
```

Výstup:

- `keys/app_sign.bin` – binárny ML-DSA-44 podpis (2420 B)

---

### 🔍 `verify`

```bash
./verify <subor_na_overenie> <publickey.bin> <signature.bin>
# Príklad:
./verify files/test_bin.bin keys/app_pk.bin keys/app_sign.bin
```

Overí podpis pomocou ML-DSA-44 verejného kľúča.

---

### 🔐 `openssl_from_app` – Export APP → OpenSSL

```bash
./openssl_from_app <seed.bin | seed.hex>
```

Podporované formáty seedu:

- `seed.bin` → 32 bajtov  
- `seed.hex` → 64 hex znakov (bez medzier / s medzerami / s dvojbodkami)

Program spraví:

1. Načíta seed  
2. Spustí deterministické generovanie OpenSSL kľúča:
   ```bash
   openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed_hex>
   ```
3. Extrahuje z OpenSSL:

   - `keys/openssl_app_key.pem`  
   - `keys/openssl_app_pk.pem`  
   - `keys/openssl_appkey_dump.txt`  
   - `keys/openssl_app_seed.bin`  
   - `keys/openssl_app_sk.bin`  
   - `keys/openssl_app_pk.bin`  

---

### 🔄 `app_from_openssl` – Import OpenSSL → APP

```bash
./app_from_openssl <openssl_pem_subor>
# Ak sa argument neuvedie, predvolene:
# ./app_from_openssl keys/openssl_key.pem
```

Extrahuje z PEM → formátu mojej aplikácie:

- `keys/app_openssl_seed.bin` – 32 B seed  
- `keys/app_openssl_seed.hex` – textová hexa verzia seedu  
- `keys/app_openssl_sk.bin` – secret key v PQClean formáte (2560 B)  
- `keys/app_openssl_pk.bin` – public key v PQClean formáte (1312 B)  

---

## 🧰 Základné OpenSSL príkazy

### Generovanie zo seedu

```bash
openssl genpkey -algorithm ML-DSA-44     -pkeyopt hexseed:<seed_hex>     -out keys/openssl_app_key.pem
```

### Dump do textu

```bash
openssl pkey -in keys/openssl_app_key.pem -text -noout     > keys/openssl_appkey_dump.txt
```

### Export privátneho a verejného kľúča

```bash
openssl pkey -in keys/openssl_app_key.pem     -out keys/openssl_app_sk.pem

openssl pkey -in keys/openssl_app_key.pem -pubout     -out keys/openssl_app_pk.pem
```

### Extrakcia raw blokov z dump súboru (alternatíva k môjmu C kódu)

```bash
# seed (32 B)
grep -A 3 "^seed" keys/openssl_appkey_dump.txt   | grep -v "seed" | tr -d ' 
:'   | xxd -r -p > keys/openssl_seed.bin

# secret key (2560 B)
awk '/^priv:/ {flag=1; next} /^pub:/ {flag=0} flag' keys/openssl_appkey_dump.txt   | tr -d ' 
:' | xxd -r -p > keys/openssl_secretkey.bin

# public key (1312 B)
grep -A 999 "^pub" keys/openssl_appkey_dump.txt   | grep -v "pub" | tr -d ' 
:'   | xxd -r -p > keys/openssl_publickey.bin
```

---

## ✉️ Podpisovanie / overovanie cez OpenSSL

ML-DSA-44 v OpenSSL **nepodporuje explicitný digest** (nie je možné použiť `-digest` ani `-md` ako pri RSA/ECDSA).  
Preto treba použiť *raw* režim:

### Podpis (OpenSSL)

```bash
openssl pkeyutl -sign     -inkey keys/openssl_app_sk.pem     -rawin     -in files/test_bin.bin     -out keys/openssl_app_sign.sig
```

### Overenie (OpenSSL)

```bash
openssl pkeyutl -verify -pubin     -inkey keys/openssl_app_pk.pem     -rawin     -in files/test_bin.bin     -sigfile keys/openssl_app_sign.sig
```

Ak je podpis správny, OpenSSL vypíše:

```text
Signature Verified Successfully
```

---

## ✅ Overovanie podpisov v ML-DSA-44 (APP, OpenSSL, kombinácie)

ML-DSA-44 je deterministická schéma založená na mriežkach, ktorá **nepoužíva digest (hash) mimo špecifikácie algoritmu**.  
OpenSSL aj moja aplikácia používajú **rovnaký binárny formát podpisu**, takže podpisy sú 100% **zameniteľné**.

Nasledujúce sekcie ukazujú, ako overovať podpisy vo všetkých kombináciách.

---

### 🟦 1. APP → APP

Podpis vytvorený mojou aplikáciou sa overuje mojou aplikáciou.

#### Podpis

```bash
./sign_file files/test_bin.bin keys/app_sk.bin
# výstup: keys/app_sign.bin
```

#### Overenie

```bash
./verify files/test_bin.bin keys/app_pk.bin keys/app_sign.bin
```

Výstup:

```text
Podpis je platny pre subor: files/test_bin.bin
```

---

### 🟩 2. OpenSSL → OpenSSL

Podpis vytvorený OpenSSL sa overuje v OpenSSL.

#### Podpis

```bash
openssl pkeyutl -sign     -inkey keys/openssl_app_sk.pem     -rawin     -in files/test_bin.bin     -out keys/openssl_app_sign.sig
```

#### Overenie

```bash
openssl pkeyutl -verify -pubin     -inkey keys/openssl_app_pk.pem     -rawin     -in files/test_bin.bin     -sigfile keys/openssl_app_sign.sig
```

---

### 🟧 3. APP → OpenSSL

Podpis vytvorený mojou aplikáciou vie verifikovať OpenSSL.

#### Podpis z APP

```bash
./sign_file files/test_bin.bin keys/app_sk.bin
# výstup: keys/app_sign.bin
```

#### Overenie cez OpenSSL

```bash
openssl pkeyutl -verify -pubin     -inkey keys/openssl_app_pk.pem     -rawin     -in files/test_bin.bin     -sigfile keys/app_sign.bin
```

Ak je všetko v poriadku:

```text
Signature Verified Successfully
```

---

### 🟥 4. OpenSSL → APP

Podpis vytvorený OpenSSL vie verifikovať moja aplikácia.

#### Podpis z OpenSSL

```bash
openssl pkeyutl -sign     -inkey keys/openssl_app_sk.pem     -rawin     -in files/test_bin.bin     -out keys/openssl_app_sign.sig
```

#### Overenie cez APP

```bash
./verify files/test_bin.bin keys/app_pk.bin keys/openssl_app_sign.sig
```

Výstup:

```text
Podpis je platny pre subor: files/test_bin.bin
```

---

### 🔄 5. Kompatibilita v skratke

| Podpis vytvorený | Overenie v… | Funguje? |
|------------------|-------------|----------|
| APP              | APP         | ✔ Áno    |
| OpenSSL          | OpenSSL     | ✔ Áno    |
| APP              | OpenSSL     | ✔ Áno    |
| OpenSSL          | APP         | ✔ Áno    |

✔ Identický binárny formát podpisu (2420 B)  
✔ Identický formát public key (1312 B)  
✔ Identický formát secret key (2560 B)  
✔ Identická logika generovania z 32B seedu

---

## 🧪 Porovnanie kľúčov a podpisov (APP ↔ OpenSSL)

ML-DSA-44 je deterministický algoritmus.  
To znamená, že ak použijeme **rovnaký 32-bajtový seed**, potom:

✔ moja aplikácia aj OpenSSL musia vygenerovať **identický**:

- `ρ`  
- `ρ'`  
- `key`  
- `s1`, `s2`, `t0`, `t1`  
- **public key (1312 B)**  
- **secret key (2560 B)**  

---

### 🟦 1. Porovnanie kľúčov (APP ↔ OpenSSL)

#### Public key (1312 B)

```bash
cmp keys/app_pk.bin keys/openssl_app_pk.bin
# žiadny výstup → súbor je identický
```

Alternatívne:

```bash
diff -s keys/app_pk.bin keys/openssl_app_pk.bin
# Files ... and ... are identical
```

Hexdump kontrola:

```bash
xxd keys/app_pk.bin > pk_app.hex
xxd keys/openssl_app_pk.bin > pk_openssl.hex
diff -s pk_app.hex pk_openssl.hex
```

---

#### Secret key (2560 B)

```bash
cmp keys/app_sk.bin keys/openssl_app_sk.bin
```

Hexdump kontrola:

```bash
xxd keys/app_sk.bin | head
xxd keys/openssl_app_sk.bin | head
```

---

#### Seed (32 B)

```bash
cmp keys/app_seed.bin keys/openssl_app_seed.bin
# bez výstupu → rovnaký seed
```

Ak seed sedí, všetky kľúče musia byť identické.

---

### 🟥 2. Porovnanie podpisov (APP ↔ OpenSSL)

Podpisy ML-DSA-44 sú **deterministické**.  
Pri rovnakom:

- message (bitovo identický súbor),
- secret key (identický binárny obsah)

musí platiť:

👉 **APP podpis = OpenSSL podpis**

---

#### 2.1 Podpis vytvorený APP a OpenSSL musí byť rovnaký

Podpis z APP:

```bash
./sign_file files/test_bin.bin keys/app_sk.bin
mv keys/app_sign.bin keys/app_sign_app.bin
```

Podpis z OpenSSL:

```bash
openssl pkeyutl -sign     -inkey keys/openssl_app_sk.pem     -rawin     -in files/test_bin.bin     -out keys/app_sign_openssl.bin
```

Porovnanie:

```bash
cmp keys/app_sign_app.bin keys/app_sign_openssl.bin
# žiadny výstup → podpisy sú identické
```

---

#### 2.2 Hexdump podpisov

```bash
xxd keys/app_sign_app.bin | head
xxd keys/app_sign_openssl.bin | head
```

Ak sú podpisy rovnaké, uvidíš identické hexdumpy.

---

### 🧠 3. Kedy podpisy NEBUDÚ zhodné?

Podpisy budú rozdielne, ak:

- použiješ **iný seed**,  
- použiješ **iný secret key** (hoci aj o 1 bajt),  
- zmeníš čo i len 1 bit v súbore,  
- súbor má rozdielne konce riadkov (LF vs CRLF) alebo BOM,  
- editor re-encoderuje obsah (napr. UTF-8 vs Latin-2).

---

### 🧠 4. Kedy podpisy MUSIA byť identické?

Podpisy budú 1:1 rovnaké, ak:

- message je bitovo totožný,  
- raw PQClean secret key je identický,  
- seed je rovnaký → kľúče sú rovnaké,  
- obidve implementácie (APP aj OpenSSL) používajú rovnaký ML-DSA-44 (FIPS 204).

Tieto podmienky tento projekt spĺňa.

---

### 🎯 5. Rýchly komplet test kompatibility

```bash
./genkey
./openssl_from_app keys/app_seed.bin

./sign_file files/test_bin.bin keys/app_sk.bin
openssl pkeyutl -sign   -inkey keys/openssl_app_sk.pem   -rawin   -in files/test_bin.bin   -out keys/sig_openssl.bin

cmp keys/app_pk.bin keys/openssl_app_pk.bin
cmp keys/app_sk.bin keys/openssl_app_sk.bin
cmp keys/app_sign.bin keys/sig_openssl.bin
```

Ak všetky `cmp` nič nevypíšu → **APP ↔ OpenSSL kompatibilita je 100% OK**.

---

## 📁 Štruktúra projektu

```text
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

## 🚀 Rýchle príklady použitia

### Generovanie APP kľúčov

```bash
./genkey
```

### Podpis v APP

```bash
./sign_file files/test_bin.bin keys/app_sk.bin
# výstup: keys/app_sign.bin
```

### Overenie v APP

```bash
./verify files/test_bin.bin keys/app_pk.bin keys/app_sign.bin
```

### Identické OpenSSL kľúče z toho istého seedu

```bash
./openssl_from_app keys/app_seed.bin
```

### Import PEM → APP formátu

```bash
./app_from_openssl keys/openssl_app_key.pem
```

---

## 🧾 Poznámky

- Testované na **OpenSSL 3.5.2 / 3.6.0**
- Implementácia rešpektuje **FIPS 204 (ML-DSA)**  
- APP ↔ OpenSSL kompatibilita: kľúče aj podpisy sú zameniteľné  
- Export/import funguje obojsmerne (APP → OpenSSL aj OpenSSL → APP)  
- Príklady sú pripravené pre použitie v rámci diplomovej práce a dokumentácie

---

## 👤 Autor

**Dávid Mudrák**  
Diplomová práca: *Post-kvantové digitálne podpisy (ML-DSA-44)*  
TUKE – FEI, Počítačové siete
