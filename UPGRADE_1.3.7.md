# Upgrade Guide: esp32-homekit 1.3.3 → 1.3.7

This upgrade introduces important internal changes and should be treated as a migration.

## ✅ Required steps

1. **Upgrade ESP-IDF first (minimum `v5.5.2`)**
   - Do this before updating the HomeKit library.
   - Recommended order:
     1. Update ESP-IDF
     2. Build the current project
     3. Upgrade `esp32-homekit` to `1.3.7`

2. **Fix enum renames**
   - Several accessory category enum names changed from plural to singular.
   - Search and replace occurrences of:
     - `homekit_accessory_categories_`
     - with `homekit_accessory_category_` variants that match your accessory type.

3. **Make `.category` explicit in `HOMEKIT_ACCESSORY(...)`**
   - Do not rely on old implicit defaults.
   - Set category explicitly for each accessory, for example:

   ```c
   HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_light, .services=(homekit_service_t*[]){
       HOMEKIT_SERVICE(ACCESSORY_INFORMATION, ...),
       HOMEKIT_SERVICE(LIGHTBULB, ...),
       NULL
   });
   ```

4. **Retest complete pairing lifecycle**
   - Test at least:
     - Pair
     - Reboot accessory
     - Reconnect from controller
     - Remove pairing
     - Pair again

5. **Validate request size limits**
   - New limits to validate against your controller integrations:
     - Max characteristics per request: **64**
     - Max JSON body: **8192 bytes**

## ⭐ Strongly recommended

6. **Use runtime notify config API when metadata changes dynamically**

```c
homekit_server_notify_config_changed();
```

7. **Measure memory impact**
   - Check free heap:
     - Before HomeKit init
     - After HomeKit init
     - After first successful pairing

8. **Test with clean NVS storage**
   - Erase flash / NVS as part of validation.
   - Pair from scratch and verify stable behavior.

## ❌ Avoid these mistakes

- Upgrading library while staying on older ESP-IDF versions.
- Fixing enum compile errors without verifying the semantic category mapping.
- Assuming storage behavior is backward-compatible without testing.
- Rolling directly to production devices without staging validation first.

## ⚠️ High-risk scenarios (extra regression testing advised)

Run deeper tests if one or more apply:

- Custom JSON endpoints
- Direct TLV manipulation
- Manual storage manipulation
- Non-standard HomeKit controllers
- Tight-memory targets (ESP32-C2/C3)

## Quick checklist

1. Upgrade ESP-IDF (>= 5.5.2)
2. Build project
3. Upgrade library
4. Fix enums
5. Set explicit accessory categories
6. Test pairing + reboot + reconnect
7. Test automations/controllers
8. Test bulk requests
9. Deploy to production only after staging pass

## Professional verdict

`1.3.7` is a meaningful improvement over `1.3.3`, but this is not a trivial patch-level upgrade in practice. Plan it as a **migration-worthy minor upgrade**.
