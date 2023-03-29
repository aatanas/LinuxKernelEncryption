# LinuxKernelEncryption

Cilj ovog projekta je obezbediti mehanizam za enkripciju i dekripciju za Linux pomoću transpozicione šifre. Projekat je napisan u C i assembly unutar Linux kernela.
Enkriptovani fajlovi ostaju enkriptovani i nakon gašenja i ponovnog paljenja
sistema. Sistem podržava sledeće funkcionalnosti:
- Korisnik može da enkriptuje / dekriptuje datoteke.
- Čitanje i pisanje pomoću standardnih sistemskih poziva funkcioniše nad
enkriptovanim datotekama, kao i nad normalnim datotekama.
- Korisnik može da podešava ključ pomoću kojeg se vrši enkripcija / dekripcija.
- Korisnik može da generiše nasumični ključ.
- Enkripcija direktorijuma
  - Podrazumeva enkripciju svih datoteka unutar direktorijuma, kao i datoteka u
poddirektorijumima.
- Keširanje ključa na određeno vreme, lokalno i globalno. Keš se ne nasleđuje kod child
procesa.
- Pamćenje ključa za enkriptovane datoteke, kako bi se izbeglo kvarenje datoteka
upotrebom pogrešnog ključa.
- Alat za postavljanje ključa koji ne ispisuje ključ na ekranu.

#### Kriptografski algoritam
Za potrebe enkriptovanja / dekriptovanja koristi se transpozicioni algoritam zasnovan na
zameni kolona matrice, koji je opisan [ovde](http://practicalcryptography.com/ciphers/columnar-transposition-cipher/).

Datoteka se enkriptuje u blokovima od po 1024 bajta (čitav buffer_head bafer koji
se inače koristi za čitanje i pisanje datoteka). Kao ključ se koristi niz printabilnih ASCII
karaktera čija ASCII vrednost se koristi za transpoziciju. Dužina ključa će uvek biti neki
stepen dvojke i manja od 1024, tako da matrica sa 1024 karaktera sigurno može da se
formira sa tim brojem kolona. Ključ će uvek biti bez ponavljanja.

Spisak enkriptovanih fajlova se zapisuje na disku. Ovu datoteku / blok nikako nije
moguće obrisati, prepisati, izmeniti, i sl. osim normalnim radom sistema za enkripciju /
dekripciju.

Nakon što korisnik prvi put podesi ključ za enkripciju, postaje moguće čitati i pisati
enkriptovane datoteke. One mogu već da budu na disku, a mogu i da se obične datoteke
enkriptuju pomoću posebne komande. Funkcije read i write rade normalno i sa
enkriptovanim i sa ne-enkriptovanim datotekama.

#### Alati
##### *keyset \<kljuc>*
Postavlja prosleđeni parametar kao trenutno aktivan globalan ključ. Dužina ključa mora da
bude stepen dvojke i manji od 1024. Ako nije, prijavljuje se greška. Pri prvom pozivu ovog alata se
čita spisak enkriptovanih fajlova sa diska, i smešta se u memoriju. Nakon toga, svaki poziv
*read* ili *write* funkcija nad enkriptovanom datotekom se uspešno čita, odnosno piše, tj. 
primenjuje se algoritam za enkripciju / dekripciju pri izvršavanju tih poziva.

##### *encr \<file>*
Enkriptuje zadati fajl pomoću zadatog ključa, u slučaju da nije već enkriptovan. Ako je fajl
već enkriptovan, prijavljuje se greška. Ovaj alat takođe u sistemu zabeležuje da je ovaj fajl
enkriptovan. Ako ključ nije prethodno postavljen pomoću alata keyset, prijavljuje se greška.

##### *decr \<file>*
Dekriptuje zadati fajl pomoću zadatog ključa, u slučaju da je fajl prethodno enkriptovan. Ako
fajl nije prethodno enkriptovan, prijavljuje se greška. Ovaj alat u sistemu zabeležuje da ovaj 
fajl više nije enkriptovan. Ako ključ nije prethodno postavljen pomoću alata keyset,
prijavljuje se greška.

##### *keyclear*
Resetuje ključ na NULL, i onemogućava dalje enkriptovanje i dekriptovanje sve dok se ne
pozove keyset ponovo. Funkcije *read* i *write* ponovo rade kao da nisu svesne
enkripcije.

##### *keygen level*
Generiše nasumični ključ sačinjen od printabilnih ASCII karaktera i ispisuje ga na ekranu.
Parametar level može da bude jedno od:
- 1 - generiše se ključ dužine 4;
- 2 - generiše se ključ dužine 8;
- 3 - generiše se ključ dužine 16.
Ako level nije dato, ili nije jedna od ove tri vrednosti, prijavljuje se greška.

#### Enkripcija / dekripcija direktorijuma
Ako korisnik izvrši sistemski poziv za enkriptovanje (pomoću alata encr) nad
direktorijumom, dešava se sledeće:
- Enkriptuju se sve datoteke unutar direktorijuma, kao i sam direktorijum.
- Za svaki poddirektorijum unutar zadatog direktorijuma se ponavlja ista operacija.

Ova logika je ugrađena u sam sistemski poziv, ne u alat *encr*. Sistemski poziv i
dalje ne funkcioniše ako ključ nije setovan.

Alat i sistemski poziv *decr* radi na isti ovaj način kada je pozvan nad
direktorijumom.

#### Keširanje ključa na ograničeno vreme
Sistemski poziv *keyset* može da postavi globalni ili lokalni ključ. Svaki
proces ima svoj lokalni ključ koji može da postavi pomoću ovako modifikovanog
keyset poziva. keyset tool i dalje postavlja globalni ključ.

Za svaki proces sistem pamti kad je zadnji put pozvao *keyset* i ako prođe ordeđeno vreme
(default 45 sec) sistem automatski briše ključ za taj proces (kao da je pozvan
sistemski poziv *keyclear* koji radi lokalno).

Globalni ključ se prazni 2 min posle postavljanja.

*keyclear* poziv čisti ili globalni ili lokalni ključ, dok keyclear tool čisti globalni.

Fork-ovani procesi ne nasleđuju keširan ključ roditelja.

#### Pamćenje ključa za enkriptovane datoteke
Pri enkriptovanju datoteke beleži se i hash vrednost ključa sa kojim je ona enkriptovana. 
Ako se pokuša pristup datoteci (decr / read / write) sa pogrešnim
ključem, umesto dekriptovanja vraća se kod o grešci -EINVAL.

Pri dekriptovanju će operacija biti uspešna samo nad datotekama
koje su enkriptovane sa trenutno postavljenim ključem.

#### Modifikacija keyset alata
Pri postavljanju ključa pomoću keyset alata, ključ se više ne zadaje kao argument na
komandnoj linji. Alat se startuje bez argumenata, i od korisnika se očekuje da unese ključ po
startovanju alata. Karakteri koje korisnik unosi se ne prikazuju na ekranu za vreme
unosa.
