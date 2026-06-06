# Code Audit Report — 2026-06-06

## Scope
Diepe, volledige review van de C-kern in `src/` en de publieke headers in `include/homekit/`,
met nadruk op geheugenveiligheid, parsercorrectheid en de beveiligingskritische pairing-/
sessieflows (SRP, Curve25519, Ed25519, ChaCha20-Poly1305). De `dist/`-map (oude releases)
is buiten scope gelaten.

Reviewmethode: handmatige regel-voor-regel lezing van alle kernmodules
(`server.c`, `crypto.c`, `tlv.c`, `json.c`, `storage.c`, `accessories.c`, `port.c`,
`base64.c`, `bitset.c`, `query_params.c`), met focus op alle paden die door
onvertrouwde netwerkinvoer worden geraakt.

Ernst-legenda: **H** = High, **M** = Medium, **L** = Low.

---

## Bevindingen

### H1 — Oneindige lus in `GET /characteristics` (DoS)
**Bestand:** `src/server.c`, `homekit_server_on_get_characteristics`, regels ~2655–2688

De tweede verwerkingslus verhoogt `id_index` alleen onderaan de body (regel 2687). De twee
`continue`-takken voor "characteristic niet gevonden" (2665) en "niet leesbaar" (2671)
springen terug naar de lus-conditie **zonder** `id_index` te verhogen.

```c
while (id_index < countof(... ids) && ids[id_index].aid != 0) {
    ch = ...by_aid_and_iid(aid, iid);
    if (!ch)            { write_error(...); continue; }  // id_index ongewijzigd → loopt eeuwig
    if (!readable)      { write_error(...); continue; }  // idem
    ...
    id_index++;
}
```

**Impact:** elke gepairde controller die één niet-bestaande of write-only `aid.iid` opvraagt
laat de servertaak vastlopen en blijft eindeloos JSON naar de buffer schrijven. Pakket-DoS van
het hele accessoire. De eerste validatielus (2613–2632) heeft dit probleem niet omdat die de
index bovenaan verhoogt.

**Fix:** verhoog `id_index` bovenaan de lus (zoals in de eerste lus), of vervang de `continue`s
door correcte ophoging.

---

### H2 — Onbegrensde array-write bij parsen van `id=`-queryparameter
**Bestand:** `src/server.c`, `homekit_server_on_url`, regels ~3442–3492 (struct op regel 168)

`endpoint_params.ids` is een vaste array van **25** elementen. De parser van
`/characteristics?id=a.b,c.d,...` schrijft `ids[id_count].aid/iid` en doet `id_count++` zonder
ooit te controleren of `id_count < countof(ids)`:

```c
server->endpoint_params.ids[id_count].aid = aid;
server->endpoint_params.ids[id_count].iid = iid;
id_count++;   // geen bovengrens
```

**Impact:** een verzoek met meer dan 25 komma-gescheiden id's schrijft buiten de array, dus
in aangrenzende velden van de heap-gealloceerde `homekit_server_t` (o.a. `format`, `data[]`).
Out-of-bounds write / geheugencorruptie. Post-auth (versleutelde sessie nodig), maar een
gecompromitteerde of kwaadwillende controller kan hiermee de serverstaat corrumperen.

**Fix:** breek de lus af bij `id_count >= countof(server->endpoint_params.ids)`.

---

### H3 — Onbegrensde / overloopbare body-allocatie (pre-auth DoS, mogelijke heap-overflow)
**Bestand:** `src/server.c`, `homekit_server_on_body`, regels 3537–3555

```c
context->server->body = malloc(length + parser->content_length + 1);
// ... geen NULL-check ...
memcpy(context->server->body + context->server->body_length, data, length);
```

Drie problemen:
1. **Geen NULL-check** na `malloc`. Een grote `Content-Length` laat `malloc` falen; de
   daaropvolgende `memcpy` dereferentieert `NULL` → crash. Bereikbaar **zonder authenticatie**
   via `POST /pair-setup` (wordt onversleuteld verwerkt).
2. **Geen bovengrens** op de body-grootte → geheugenuitputting-DoS.
3. **Integer-truncatie:** `parser->content_length` is `uint64_t`; de som wordt afgekapt naar
   32-bits `size_t`. Een `Content-Length` van bv. `0x100000001` levert een minieme allocatie op,
   waarna instromende body-bytes via `memcpy` buiten de buffer worden geschreven → heap-overflow.

**Fix:** valideer `parser->content_length` tegen een redelijk maximum (bv. enkele KB), controleer
op `NULL`, en gebruik overflow-veilige optelling.

---

### H4 — Ontbrekende grenscontroles in `tlv_parse` (OOB-read op onvertrouwde TLV)
**Bestand:** `src/tlv.c`, `tlv_parse`, regels 221–262

`tlv_parse` verwerkt door de aanvaller gecontroleerde TLV-data (pair-setup/verify, /pairings,
characteristic-writes). Diverse leesoperaties controleren de buffergrens onvoldoende:

```c
while (j < length && buffer[j] == type && buffer[j+1] == 255)  // leest buffer[j+1], alleen j<length gecheckt
...
if (j < length && buffer[j] == type) { chunk = buffer[j+1]; }   // idem: j+1 kan == length zijn
...
memcpy(p, &buffer[i+2], chunk_size);   // geen check dat i+2+chunk_size <= length
```

**Impact:** een afgekapte of misvormde TLV veroorzaakt out-of-bounds reads (één byte voorbij,
of een hele chunk voorbij het einde) en kan de kopieerlus laten doorlopen voorbij de buffer.
Informatielek/crash op een pre-auth pad.

**Fix:** controleer overal `j + 1 < length` vóór het lezen van het lengtebyte, en valideer dat
`i + 2 + chunk_size <= length` vóór elke `memcpy`. Behandel de retourwaarde van
`tlv_add_value_` (nu genegeerd → geheugenlek bij falen).

---

### M1 — `crypto_ed25519_verify` behandelt interne fout als geldige handtekening
**Bestand:** `src/crypto.c`, regels 475–487

```c
int verified;
int r = wc_ed25519_verify_msg(sig, sigSz, msg, msgSz, &verified, key);
return !r && !verified;
```

Waarheidstabel (caller doet `if (r) faal`):
- Geldig: `r=0, verified=1` → retour `0` → geaccepteerd ✓
- Ongeldig: `r=0, verified=0` → retour `1` → geweigerd ✓
- **Interne fout: `r≠0`** → `!r==0` → retour `0` → **geaccepteerd** ✗ (en `verified` is
  ongeïnitialiseerd → UB)

**Impact:** als wolfSSL een fout teruggeeft (bv. misvormde invoer), wordt de handtekening als
geldig beschouwd. Gebruikt in pair-setup stap 3 (1684) en pair-verify stap 2 (2422) — de
kern van de authenticatie.

**Fix:** `return r ? <fout> : (verified ? 0 : <fout>)`; initialiseer `verified = 0` en
behandel iedere niet-succes als verificatiefout.

---

### M2 — Heap-overflow in `homekit_characteristic_clone` (valid_values_ranges)
**Bestand:** `src/accessories.c`, regels 294–303

Allocatie gebruikt de juiste structgrootte, maar de kopie gebruikt de **pointergrootte**:

```c
// reserve: align_size(sizeof(homekit_valid_values_range_t) * count)   // 2 bytes/elem
memcpy(clone->..ranges, ch->..ranges, sizeof(homekit_valid_values_range_t*) * c);  // 4 bytes/elem
p += align_size(sizeof(homekit_valid_values_range_t*) * c);                          // 4 bytes/elem
```

`homekit_valid_values_range_t` is 2 bytes (`{uint8_t start, end;}`); een pointer is 4 bytes op
ESP32. De `memcpy` schrijft `4*count` bytes in een `2*count`-buffer en leest `2*count` bytes
voorbij de bron → heap-overflow + OOB-read, plus verkeerde `p`-uitlijning voor daaropvolgende
velden (callbacks).

**Fix:** gebruik `sizeof(homekit_valid_values_range_t)` in zowel de `memcpy`-omvang als de
`p +=`-stap.

---

### M3 — Device-identifier lengte niet gevalideerd vóór string-operaties
**Bestand:** `src/server.c` (1701, 2380, 3206…) + `src/storage.c`

De TLV-waarde `tlv_device_id->value` is een ruwe, niet-nulgetermineerde bytebuffer. Hij wordt
direct doorgegeven aan storagefuncties die `strncpy(dst, id, DEVICE_ID_SIZE)` /
`strncmp(..., DEVICE_ID_SIZE)` (= 36) doen. Is de TLV-waarde korter dan 36 bytes, dan wordt tot
36 bytes voorbij de buffer gelezen. De code erkent dit zelf: `// TODO: check that
tlv_device_id->size == 36` (regel 1602).

**Impact:** out-of-bounds heap-read, mogelijk informatielek; op een pre-auth pad (pair-setup
stap 5).

**Fix:** verifieer `tlv_device_id->size == DEVICE_ID_SIZE` (of ≤ en kopieer exact `->size`)
vóór gebruik.

---

### M4 — Ontbrekende NULL-checks en resource-lekken in `crypto.c`
**Bestand:** `src/crypto.c`

- `crypto_srp_new` (105): `malloc(sizeof(Srp))` zonder NULL-check; `srp` wordt direct aan
  `wc_SrpInit` gegeven. Bij init-fout retourneert hij `NULL` **zonder** `srp` te `free`en → lek.
- `crypto_srp_init` (155): `malloc(verifierLen)` zonder NULL-check vóór `wc_SrpGetVerifier`.
- `crypto_ed25519_generate` (389) en `crypto_curve25519_generate` (514): `wc_InitRng(&rng)`
  zonder bijbehorende `wc_FreeRng(&rng)` → resource-/entropielek; bovendien wordt de sleutel
  dubbel geïnitialiseerd (`crypto_*_init` wordt opnieuw aangeroepen op een al-geïnitialiseerde
  sleutel).
- `crypto_chacha20poly1305_encrypt` (332) retourneert `-1` bij te kleine buffer waar de rest
  van de module `-2` gebruikt (inconsistente conventie).

**Fix:** NULL-checks toevoegen; `wc_FreeRng` aanroepen; init-falen netjes opruimen.

---

### M5 — `client_decrypt` schrijft elk blok naar buffer-begin
**Bestand:** `src/server.c`, regel 1057

```c
crypto_chacha20poly1305_decrypt(... decrypted, &decrypted_len);  // moet decrypted + decrypted_offset zijn
decrypted_offset += decrypted_len;   // offset wordt bijgehouden maar nooit gebruikt
```

Bij payloads met meerdere 1024-byte blokken overschrijft elk blok het vorige. Momenteel
**latent** omdat de leesbuffer `data[1024+18]` per `read()` maar één blok kan bevatten, maar het
is een verborgen correctheidsbug die toeslaat zodra de buffer groeit.

**Fix:** geef `decrypted + decrypted_offset` als uitvoerpointer.

---

### L1 — Expliciete service-/characteristic-`id` zet `iid` niet
**Bestand:** `src/server.c`, `homekit_accessories_init`, 4287–4304

Als `service->id`/`ch->id` is opgegeven, wordt de teller `iid` bijgewerkt maar
`*_infos[idx].iid` blijft `0` (calloc). `find_service_info`/`/accessories` rapporteren dan iid 0.

### L2 — `base64_decode` valideert tekens niet
**Bestand:** `src/base64.c`, 34–49, 93–115

Ongeldige tekens worden via `base64_decode_char` stil naar `0` gedecodeerd in plaats van een
fout. Misvormde invoer levert stille datacorruptie i.p.v. afwijzing.

### L3 — JSON-strings worden niet ge-escaped
**Bestand:** `src/json.c`, `json_string`, regel 299 (`// TODO: escape string`)

Een string-characteristic-waarde met `"`, `\` of stuurtekens produceert ongeldige JSON in
`/accessories` en `/characteristics`.

### L4 — `homekit_storage_init`: `magic` mogelijk ongeïnitialiseerd
**Bestand:** `src/storage.c`, 97–113

Bij een echte (niet-NOT_FOUND) leesfout logt de code een fout maar keert **niet** terug; daarna
wordt `strncmp` op een ongeïnitialiseerde stack-`magic` gedaan.

### L5 — `homekit_characteristic_clone` met `description == NULL`
**Bestand:** `src/accessories.c`, 243–246

Bij `description_len == 0` schrijft `p[description_len - 1] = 0` naar `p[-1]` en wordt
`clone->description` op een niet-NULL maar ongeldige pointer gezet i.p.v. `NULL`.

### L6 — Diverse ontbrekende NULL-checks
`homekit_value_copy` (accessories.c 126, 139–140): `tlv_new()`/`malloc` zonder NULL-check.
`homekit_characteristic_add_notify_callback` (449): `malloc` zonder NULL-check.

### L7 — Setup-wachtwoord in debug-logs
**Bestand:** `src/server.c`, 1347/1354

Het pairing-wachtwoord wordt via `CLIENT_DEBUG` gelogd. Alleen in `HOMEKIT_DEBUG`-builds, maar
een gevoelig gegeven in de logs.

### L8 — GCC nested functions
Door de hele `server.c`/`json.c` worden geneste functies gebruikt (`_do_write`,
`process_characteristics_update`, …). Dit is een GCC-extensie (werkt op xtensa-gcc) maar is
niet-standaard C en breekt onder Clang/andere toolchains.

---

## Aanbevelingen (prioriteit)
1. **Herstel H1–H4 direct** — dit zijn netwerk-bereikbare DoS/geheugencorruptie-paden, deels
   pre-auth.
2. **M1** — authenticatie-foutpad moet fail-closed zijn; dit is veiligheidskritisch.
3. Voeg een **maximale request-/body-grootte** toe en valideer alle TLV-velden op exacte
   verwachte lengtes (device-id 36, public keys 32, signatures 64).
4. Breid de host-tests (`tests/host/test_core.c`) uit met fuzz-/grenstests voor `tlv_parse`,
   `base64_decode` en de `id=`-queryparser; draai ze onder ASan/UBSan (CI-gate bestaat al).
5. Overweeg `clang-tidy`/`cppcheck` als niet-blokkerende statische analyse, en vermijd geneste
   functies voor toolchain-portabiliteit.

## Statusoverzicht
- 4 High, 5 Medium, 8 Low bevindingen.

## Remediatie (toegepast)
Alle bevindingen zijn verholpen in branch `audit-fixes-2026-06-06`:

| ID | Bestand | Fix |
|----|---------|-----|
| H1 | server.c | `id_index` wordt nu bovenaan de lus verhoogd → geen oneindige lus meer. |
| H2 | server.c | Grenscontrole `id_count >= countof(ids)` toegevoegd vóór schrijven. |
| H3 | server.c | `HOMEKIT_MAX_BODY_SIZE` (4096) + NULL-check + 64-bits overflow-veilige optelling + capaciteitsbewaking (`body_capacity`). |
| H4 | tlv.c | Grenscontroles op alle leesoperaties + `memcpy`; retourwaarde van `tlv_add_value_` afgehandeld. |
| M1 | crypto.c | `crypto_ed25519_verify` faalt nu gesloten (crypto-fout ≠ geldig); `verified` geïnitialiseerd. |
| M2 | accessories.c | `sizeof(homekit_valid_values_range_t)` i.p.v. pointergrootte in `memcpy`/stride. |
| M3 | server.c | `tlv_device_id->size == DEVICE_ID_SIZE` gecontroleerd op alle 4 call-sites. |
| M4 | crypto.c | NULL-checks (`crypto_srp_new`, verifier) + `wc_FreeRng` + consistente `-2`. |
| M5 | server.c | `client_decrypt` schrijft naar `decrypted + decrypted_offset`. |
| L1 | server.c | `*_infos[].iid` wordt nu ook gezet bij expliciete `id`. |
| L2 | base64.c | `base64_decode` weigert ongeldige tekens en misplaatste padding (`-1`). |
| L3 | json.c | `json_string` escapet `"`, `\`, stuurtekens (`\uXXXX`). |
| L4 | storage.c | `magic` zero-geïnitialiseerd vóór gebruik. |
| L5 | accessories.c | `clone->description = NULL` wanneer er geen beschrijving is. |
| L6 | accessories.c | NULL-checks in `homekit_value_copy`, `homekit_value_clone`, `add_notify_callback`. |
| L7 | server.c | Pairing-wachtwoord niet langer in debug-logs. |
| L8 | — | GCC nested functions: gedocumenteerd; ongewijzigd (vereist xtensa-gcc, zoals de build al gebruikt). |

**CI-uitbreiding:**
- `host-regression-tests.yml` compileert nu ook `tlv.c` en draait nieuwe regressietests
  (base64-validatie, `tlv_parse`-grenzen) onder ASan/UBSan met leak-detectie.
- Nieuwe workflow `static-analysis.yml` draait `cppcheck` (gate op `error`-severity voor de
  draagbare kernmodules, plus een informatieve volledige scan).
