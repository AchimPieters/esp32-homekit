# Code Audit Report — 2026-06-06 (ronde 2)

## Scope
Vervolgaudit op `main` (`d971505`, ná de fixes van ronde 1). Nadruk op modules die in
ronde 1 niet diep zijn bekeken: `homekit_mdns.c`, `debug.c`, `homekit_mdns_debug.c`, de
publieke macro's in `include/homekit/types.h`, en de concurrency-/lockpaden en logische
operatoren in `server.c`. Tevens herverificatie dat de fixes van ronde 1 correct zijn.

Ernst-legenda: **H** = High, **M** = Medium, **L/Info** = Low/informatief.

---

## Nieuwe bevindingen

### N1 — High: deadlock in `homekit_characteristic_notify`
**Bestand:** `src/server.c` (~regel 3904–3909)

```c
xSemaphoreTake(server->notification_lock, portMAX_DELAY);

characteristic_info_t *ch_info = find_characteristic_info_by_characteristic(server, ch);
if (!ch_info) {
        ERROR("Trying notify on unknown characteristic \"%s\"", ch->description);
        return;          // <-- keert terug ZONDER xSemaphoreGive
}
```

De fouttak keert terug terwijl `notification_lock` nog vastgehouden wordt. Elke volgende
`homekit_characteristic_notify` én `homekit_server_process_notifications` blokkeert dan
voorgoed op `xSemaphoreTake(..., portMAX_DELAY)` → de HomeKit-servertaak hangt volledig.

**Trigger:** een accessoire dat `homekit_characteristic_notify()` aanroept op een
characteristic die niet in de geregistreerde accessoireboom zit (bijv. een losse of
dynamisch aangemaakte characteristic). Goed bedoelde gebruikerscode kan dit raken.

**Fix:** `xSemaphoreGive(server->notification_lock);` vóór de `return`.

---

### N2 — Medium: logische `&&` i.p.v. bitwise `&` in `ev`-permissiecontrole
**Bestand:** `src/server.c` (~regel 3078), `homekit_server_on_update_characteristics`

```c
if (!(ch->permissions && homekit_permissions_notify)) {
        ... "notifications are not supported" ...
}
```

`homekit_permissions_notify` is een vaste, niet-nul constante, dus de uitdrukking is
feitelijk `(ch->permissions != 0)`. De bedoelde controle "heeft deze characteristic de
notify-permissie?" werkt daardoor niet: de tak slaat alleen aan als `permissions == 0`.

**Impact:** een gepairde client kan zich abonneren (`"ev": true`) op een characteristic
die de notify-permissie **niet** heeft, zolang die characteristic enige andere permissie
heeft. Vervolgens wordt `client_subscribe_to_characteristic_events` aangeroepen op een
niet-notify characteristic. Afwijking van de HAP-spec en onverwachte abonnementsstaat.

**Fix:** gebruik bitwise `&`: `if (!(ch->permissions & homekit_permissions_notify))`.
(Vergelijk de correcte vorm op regels 694, 705, 3901, 4285, 4374.)

---

### N3 — Info/Low: mDNS-pakketparser ongehard (niet in de ESP-IDF-build)
**Bestand:** `src/homekit_mdns.c` — **uitgesloten via `EXCLUDE_SRCS` in `CMakeLists.txt`**,
dus niet aanwezig in de geleverde ESP-IDF-firmware (daar gebruikt `port.c` de officiële
IDF `mdns`-component). Alleen relevant voor `ESP_OPEN_RTOS`/`HOST_BUILD`.

Twee klassieke DNS-parserrisico's in `mdns_server_process_query`:
1. **Geen header-lengtecheck:** `mdns_read_u16(data + 4/6/8)` (regels 786–788) leest vóór
   enige controle dat `data_len >= MDNS_HEADER_SIZE (12)`. Een UDP-pakket < 12 bytes geeft
   een out-of-bounds read.
2. **Decompressie-lus:** `mdns_match_label` (regel 367) volgt naam-compressiepointers
   (`0xC0`). Een pakket met een pointer die naar zichzelf (of een cyclus) wijst kan een
   oneindige lus veroorzaken — DoS. Er is geen teller/bezochte-offsetbewaking.

**Aanbeveling:** als dit pad ooit weer gebouwd wordt, valideer `data_len >= 12` aan het
begin van `mdns_server_process_query` en begrens het aantal te volgen compressiepointers.

---

## Herverificatie ronde 1
De fixes uit `CODE_AUDIT_2026-06-06.md` zijn aanwezig en correct in `main`:
- H1/H2 (`id_index`, `id_count`-grens), H3 (`HOMEKIT_MAX_BODY_SIZE` + NULL/overflow),
  H4 (`tlv_parse`-grenzen), M1 (`crypto_ed25519_verify` fail-closed), M2 (range-`sizeof`),
  M3 (device-id lengtechecks), M4 (RNG-free/NULL), M5 (decrypt-offset), L1–L7.
- Host-tests (incl. de nieuwe base64/tlv-regressietests) bouwen en slagen.
- `HOMEKIT_ACCESSORY/SERVICE/CHARACTERISTIC`-macro's in `types.h` zijn correct (de oude
  Feb-2026 macro-bugs zijn in 3.0.0 verholpen).

`debug.c` (`text_to_stringv`/`binary_to_stringv`): buffergroottes worden correct berekend
(`\xXX` = 4 bytes gebudgetteerd); alleen actief onder `HOMEKIT_DEBUG`. Geen bevindingen.

## Statusoverzicht
- 1 High (N1), 1 Medium (N2) in code die in de ESP-IDF-firmware meedraait.
- 1 informatieve bevinding (N3) in een module die buiten de IDF-build valt.
- Geen wijzigingen aangebracht in deze audit; dit rapport is uitsluitend bevindend.
