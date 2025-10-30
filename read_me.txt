Postup generovania, exportu a overovania ML-DSA-44 kľúčov a podpisov

Príkazy fungujú s OpenSSL 3.5.2 a 3.6.0 (Linux Ubuntu 24.04, BIKS obraz).

1. Generovanie kľúčov (genkey)
   - Vygeneruje app_sk.bin, app_pk.bin, app_seed.bin, app_seed.hex
   - app_seed.bin sa používa pre OpenSSL generovanie

2. Podpisovanie (sign_file)
   sign_file <subor_na_podpisanie> <subor_so_sukromnym_klucom>

3. Overenie podpisu (verify)
   verify <subor_na_overenie> <publickey.bin> <signature.bin>

4. Generovanie kľúčov v OpenSSL zo seedu
   openssl genpkey -algorithm ML-DSA-44 -pkeyopt hexseed:<seed>
   openssl pkey -in keys/openssl_app_key.pem -text -noout > keys/openssl_appkey_dump.txt
   openssl pkey -in keys/openssl_app_key.pem -out keys/openssl_app_sk.pem
   openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem

5. Extrakcia binárnych dát z dumpu
   grep -A 3 "^seed" keys/openssl_appkey_dump.txt | grep -v "seed" | tr -d ' \n:' | xxd -r -p > keys/openssl_seed.bin
   awk '/^priv:/ {flag=1; next} /^pub:/ {flag=0} flag' keys/openssl_appkey_dump.txt | tr -d ' \n:' | xxd -r -p > keys/openssl_secretkey.bin
   grep -A 999 "^pub" keys/openssl_appkey_dump.txt | grep -v "pub" | tr -d ' \n:' | xxd -r -p > keys/openssl_publickey.bin

6. Podpis a overenie súboru pomocou OpenSSL
   openssl pkeyutl -sign -inkey keys/openssl_app_sk.pem -in files/test_bin.bin -out keys/openssl_app_sign.sig
   openssl pkeyutl -verify -pubin -inkey keys/openssl_app_pk.pem -in files/test_bin.bin -sigfile keys/openssl_app_sign.sig

7. Generovanie náhodných kľúčov v OpenSSL
   openssl genpkey -algorithm ML-DSA-44 -out keys/openssl_key.pem
   openssl pkey -in keys/openssl_key.pem -out keys/openssl_sk.pem
   openssl pkey -in keys/openssl_key.pem -pubout -out keys/openssl_pk.pem

8. Podpis a overenie v OpenSSL
   openssl pkeyutl -sign -inkey keys/openssl_sk.pem -in files/test_bin.bin -out keys/openssl_sign.sig
   openssl pkeyutl -verify -pubin -inkey keys/openssl_pk.pem -in files/test_bin.bin -sigfile keys/openssl_sign.sig
