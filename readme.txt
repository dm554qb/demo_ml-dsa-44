________________________________________________________________________________________________________________________________________________________
prikazy na generovanie klucu podla urciteho seed-u, aplikacia: openssl_frokm_seed
________________________________________________________________________________________________________________________________________________________
openssl genpkey -algorithm ML-DSA-44 -out keys/openssl_app_key.pem -pkeyopt hexseed:c7c3230109ad78acb0004fd80e6d65156ce3d7338ede2192743520d0ad113a6f
openssl pkey -in keys/openssl_app_key.pem -text -noout > keys/openssl_appkey_dump.txt
openssl pkey -in keys/openssl_app_key.pem -out keys/openssl_app_sk.pem
openssl pkey -in keys/openssl_app_key.pem -pubout -out keys/openssl_app_pk.pem
--------------------------------------------------------------------------------------------------------------------------------------------------------
podpis a overenie v OpenSSL pomocou app klucov
--------------------------------------------------------------------------------------------------------------------------------------------------------
openssl pkeyutl -sign -inkey keys/openssl_app_pk.pem -in files/test_bin.bin -out keys/openssl_app_sign.sig
openssl pkeyutl -verify -pubin -inkey keys/openssl_app_pk.pem -in files/test_bin.bin -sigfile keys/openssl_app_sign.sig
--------------------------------------------------------------------------------------------------------------------------------------------------------
prikazy na vytvorenie raw dat pre potreby porovnania binárnych dat
--------------------------------------------------------------------------------------------------------------------------------------------------------
grep -A 3 "^seed" keys/openssl_appkey_dump.txt | grep -v "seed" | tr -d ' \n:' | xxd -r -p > keys/openssl_app_seed.bin
awk '/^priv:/ {flag=1; next} /^pub:/ {flag=0} flag' keys/openssl_appkey_dump.txt | tr -d ' \n:' | xxd -r -p > keys/openssl_app_sk.bin
grep -A 999 "^pub" keys/openssl_appkey_dump.txt | grep -v "pub" | tr -d ' \n:' | xxd -r -p > keys/openssl_app_pk.bin
________________________________________________________________________________________________________________________________________________________
________________________________________________________________________________________________________________________________________________________
OPENSSL
________________________________________________________________________________________________________________________________________________________
________________________________________________________________________________________________________________________________________________________
generovanie náhodných kľúčov v openssl
________________________________________________________________________________________________________________________________________________________
openssl genpkey -algorithm ML-DSA-44 -out keys/openssl_key.pem
openssl pkey -in keys/openssl_key.pem -out keys/openssl_sk.pem
openssl pkey -in keys/openssl_key.pem -pubout -out keys/openssl_pk.pem
--------------------------------------------------------------------------------------------------------------------------------------------------------
podpis a overenie
--------------------------------------------------------------------------------------------------------------------------------------------------------
openssl pkeyutl -sign -inkey keys/openssl_pk.pem -in files/test_bin.bin -out keys/openssl_sign.sig
openssl pkeyutl -verify -pubin -inkey keys/openssl_pk.pem -in files/test_bin.bin -sigfile keys/openssl_sign.sig
________________________________________________________________________________________________________________________________________________________
________________________________________________________________________________________________________________________________________________________
