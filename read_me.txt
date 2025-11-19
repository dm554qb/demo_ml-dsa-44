================================================================================
 ML-DSA-44 – Kompilácia a prostredie
================================================================================

A. PROSTREDIE, KDE BOL PROJEKT TESTOVANÝ
--------------------------------------------------------------------------------
- Ubuntu 24.04 LTS (lokálne aj v obraze predmetu BIKS)
- OpenSSL 3.5.2 a 3.6.0
- GNU GCC 11+ (kompatibilný aj GCC 12/13)

Projekt je plne funkčný v Linuxe. Vo Windows je možné kompilovať napríklad cez
MinGW.

B. AKO PROJEKT SKOMPILOVAŤ
--------------------------------------------------------------------------------
1. Potrebné mať nainštalovaný OpenSSL 3.5+ a GCC

2. V koreňovom priečinku projektu spustite:
   make

3. Po úspešnej kompilácii sa vytvoria spustitelné súbory buď pre Linux alebo Windows:
   - genkey
   - sign_file
   - verify
   - openssl_from_app
   - app_from_openssl

4. Vytvorí sa aj priečinok "keys/" (autom.)
   Programy doň ukladajú všetky výstupné súbory.

C. POZNÁMKY
--------------------------------------------------------------------------------
- Makefile automaticky kompiluje všetky zdrojové súbory v adresároch
  "source/" a "common/".
- Projekt používa implementáciu ML-DSA-44 (FIPS 204) prevzatú z PQClean.
- Všetky programy pracujú s RAW binárnymi kľúčmi a sú kompatibilné
  s OpenSSL 3.5+.
================================================================================
