ML-DSA-44 (Post-Quantum Digital Signature)
===================================================================================
Kompilácia
-----------------------------------------------------------------------------------
Preklad pomocou "make"

Tým sa vytvoria spustiteľné súbory:
 - genkey      -> generuje verejný a súkromný kľúč
	- Vytvorí súbory:
 		- publickey.bin
 		- secretkey.bin
 - sign_file   -> podpisuje zadaný súbor a vytvorí podpis "signature.bin"
	./sign_file <vstupny_subor>
 - verify      -> overuje podpis súboru
	./verify <vstupný_súbor> <signature.bin>
 - pk2pem, sk2pem -> konvertujú kľúče do PEM formátu, kompatibilné s OpenSSL
-----------------------------------------------------------------------------------
Poskladané na OpenSSL 3.5.4, ktoré je potrebné mať buď v Linux alebo pre windows
v adresary C:/OPENSSL (alebo si treba upraviť Makefile a upraviť cesty pre 
knižnice OpenSSL).
Pracoval som na WSL Ubuntu 24.04 ktorý mam vo visual studio code a taktiež som to
odskúšal vo Winwos BIKS obraze.
-----------------------------------------------------------------------------------
Programy "pk2pem" a "sk2pem" konvertujú kľúče z .bin formátu do .pem kde sa pridáva
ASN.1 hlavička aby vedel nastroj OpenSSL pracovať s týmito kľúčmi, plánoval som to
použiť pri overovaný a podpisovaný medzi mojou implementaciou a OpenSSL, no zatiaľ
to nie je dopracované až do konca pretože OpenSSL rozpozná že sa jedná o kľúč práve 
pre konkrétnu implementáciu ML-DSA-44 no nevie ho ďalej použiť pre podpis súborov a 
následne overenie. 
Viac je to opísané v dokumente "Kompatibilita_ML-DSA-44_a_OpenSSL.pdf".