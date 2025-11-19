# 🔐 ML-DSA-44 – Generovanie, export a overovanie kľúčov a podpisov medzi OpenSSL a mojou aplikáciou

Tento projekt demonštruje **plne funkčnú implementáciu post-kvantového podpisového algoritmu ML-DSA-44 (FIPS 204)** s prepojením na **OpenSSL 3.5+**.  
Cieľom je ukázať kompatibilitu medzi implementáciou z knižnice **PQClean** a OpenSSL — generovanie, podpisovanie, overovanie a *obojsmerný export/import kľúčov aj podpisov*.

---

## 🧩 Použité prostredie

- **OpenSSL 3.5.2 / 3.6.0**
- **Ubuntu 24.04 LTS**, obraz **BIKS**
- Kompilácia cez `make`

---

# ⚙️ Prehľad implementovaných nástrojov

## 🧠 `genkey` – Generovanie APP kľúčov (PQClean)

```bash
./genkey
# Generuje ML-DSA-44 kľúčový pár pomocou PQClean a vytvorí 32B deterministický seed.
```

Výstup:

- `app_sk.bin` – secret key  
- `app_pk.bin` – public key  
- `app_seed.bin` – 32B seed  
- `app_seed.hex` – 64 hex znakov

➡️ Rovnaký seed → identické kľúče v OpenSSL.

<img width="548" height="156" alt="app_genkey" src="https://github.com/user-attachments/assets/a17be8ea-ea99-499f-b4ae-96ec2d6c8f6d" />

---

## ✍️ `sign_file` – Podpis súboru pomocou APP

```bash
./sign_file <subor> <sukromny_kluc>
# Podpis súboru ML-DSA-44 implementáciou PQClean.
```
<img width="782" height="67" alt="app_sign_file" src="https://github.com/user-attachments/assets/a8fe97df-3af3-4b31-ab45-5a5c4e21d5c5" />


---

## 🔍 `verify` – Overenie podpisu pomocou APP

```bash
./verify <subor> <verejny_kluc> <podpis>
# Overenie ML-DSA-44 podpisu.
```
<img width="860" height="36" alt="Snímka obrazovky 2025-11-19 161320" src="https://github.com/user-attachments/assets/146d43be-f973-4e23-b06a-92a9fa307646" />


---

# 🔐 `openssl_from_app` – Export APP → OpenSSL

```bash
./openssl_from_app keys/app_seed.bin
# Načíta seed → OpenSSL generuje identické kľúče.
```
<img width="709" height="240" alt="openssl_from_app_generovanie" src="https://github.com/user-attachments/assets/af5ae333-da1d-41e0-b8b5-d5c97f4ea15b" />

---

# 🔄 `app_from_openssl` – Import OpenSSL → APP

```bash
./app_from_openssl keys/openssl_key.pem
# Extrahuje OpenSSL PEM → PQClean key format.
```
<img width="704" height="290" alt="app_from_openssl_generovanie" src="https://github.com/user-attachments/assets/158301dc-66e5-47ae-a231-5078c69802b0" />

---

# 🧰 OpenSSL príkazy pouzite v openssl_from_app.c

## Generovanie:

```bash
openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed_hex> -out keys/openssl_app_key.pem
```


## Dump:

```bash
openssl pkey -in keys/openssl_app_key.pem -text -noout > keys/openssl_appkey_dump.txt
```


## Podpis:

```bash
openssl pkeyutl -sign -inkey keys/openssl_app_sk.pem -rawin -in files/test_bin.bin -out keys/openssl_app_sign.sig
```

## Overenie:

```bash
openssl pkeyutl -verify -pubin -inkey keys/openssl_app_pk.pem -rawin -in files/test_bin.bin -sigfile keys/openssl_app_sign.sig
```

---

# 🧰 OpenSSL príkazy

## Generovanie kľúčov:

```bash
openssl genpkey -algorithm ML-DSA-44 -out keys/openssl_key.pem
openssl pkey -in keys/openssl_key.pem -pubout -out keys/openssl_pk.pem
```
<img width="871" height="63" alt="openssl_genkey" src="https://github.com/user-attachments/assets/0232e1ff-ebc7-4822-a684-50656f8767c2" />

## Podpis:

```bash
openssl pkeyutl -sign -inkey keys/openssl_key.pem -in files/test_bin.bin -out keys/openssl_sign.sig
```
<img width="1139" height="41" alt="openssl_sign_file" src="https://github.com/user-attachments/assets/38156b3f-e153-4eff-bf65-d7ec261e9f99" />

## Overenie:

```bash
openssl pkeyutl -verify -pubin -inkey keys/openssl_pk.pem -in files/test_bin.bin -sigfile keys/openssl_sign.sig
```
<img width="1209" height="35" alt="Snímka obrazovky 2025-11-19 161843" src="https://github.com/user-attachments/assets/3b9786ea-62a3-4015-a019-5fe9a27a5094" />

---

## 🖼️ Podpis a overenie: APP → OpenSSL

<img width="1241" src="https://github.com/user-attachments/assets/3f4cf545-1c18-4a22-8692-d05c66341395" />

**Komentár:**  
Podpis vytvorený aplikáciou (`app_sign.bin`) je úspešne overený v OpenSSL (`Signature Verified Successfully`).  
Podpis vytvorený OpenSSL (`openssl_sign.bin`) je platný aj v aplikácii.  
➡️ Tým je potvrdená kompletná obojsmerná kompatibilita.


---

## 🖼️ Podpis a overenie: OpenSSL → APP → OpenSSL

<img width="1218" src="https://github.com/user-attachments/assets/60170b4d-7ea3-4932-a078-1de82648239e" />

**Komentár:**  
OpenSSL vytvorí podpis (`openssl_sign.sig`), ktorý aplikácia úspešne overí.  
Aplikácia dokáže podpísať súbor aj kľúčmi extrahovanými z OpenSSL (`app_openssl_sk.bin`).  
➡️ OpenSSL aj APP si podpisy navzájom potvrdia.

---

# 🧪 Porovnávanie výstupov

```bash
cmp keys/app_pk.bin keys/openssl_app_pk.bin   # identické public keys
cmp keys/app_sk.bin keys/openssl_app_sk.bin   # identické secret keys
cmp keys/app_sign.bin keys/openssl_app_sign.sig  # identické podpisy
cmp keys/app_seed.bin keys/openssl_app_seed.bin  # identický seed
```

---

# 🚀 Záver

- APP ↔ OpenSSL funguje *obojsmerne*
- Rovnaký seed = identické kľúče
- Podpisy sú 100% zameniteľné
- Projekt spĺňa požiadavky pre diplomovú prácu

---

# 👤 Autor

**Dávid Mudrák**  
Diplomová práca: *Post‑kvantové digitálne podpisy (ML‑DSA‑44)*  
TUKE – FEI, Počítačové siete