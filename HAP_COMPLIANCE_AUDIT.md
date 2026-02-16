# HAP Compliance Audit and Upgrade Plan (Repository-Only)

Date: 2026-02-15  
Scope: `/workspace/esp32-homekit` source/docs review only (no Apple HAT run, no physical accessory validation).

## Short answer

To "upgrade" this repository to the latest HAP level, you need to do **two things in parallel**:

1. **Spec alignment:** update protocol behavior and advertised metadata to match the current Apple HomeKit Accessory Protocol requirements.
2. **Evidence generation:** run official Apple validation/certification tooling and keep pass artifacts.

Without both, you can improve implementation quality, but you still cannot claim letter-perfect latest HAP compliance.

## Current verdict

**This repository does not currently demonstrate "latest HAP compliance on the letter".**

It implements a substantial subset of HAP-over-IP, but the repo lacks key alignment points and formal conformance evidence.

## Gaps found in this repository

1. **mDNS protocol version default is now configurable (`HOMEKIT_MDNS_PROTOCOL_VERSION`, default `1.1`)**
   - Current code advertises `pv` via a compile-time macro instead of a fixed literal.
   - This improves alignment flexibility, but strict latest compliance still requires validation against the current Apple spec baseline.

2. **Setup payload transport flags are configurable but default to IP-only (`HOMEKIT_SETUP_PAYLOAD_FLAGS=2`)**
   - Setup URI flags are macro-driven and masked to 4 bits.
   - This can be valid for IP-only products, but must match your intended transport profile and spec constraints.

3. **Custom mDNS implementation disables IPv6**
   - `MDNS_ENABLE_IP6` is currently `0` in the custom stack.
   - If your target profile requires/assumes dual stack behavior, enable and validate IPv6 paths.

4. **Docs explicitly describe uncertified/non-commercial posture**
   - README instructs accepting uncertified accessory prompts and references non-commercial HAP material.
   - That conflicts with a strict compliance/certification-ready claim.

5. **No compliance artifacts present**
   - No Apple conformance test outputs are present in repository history/docs.

## What to do next (repo-specific upgrade checklist)

### Phase 1 — Decide target and update policy

- Select target: **non-commercial hobby build** vs **MFi/certifiable product build**.
- Add a versioned compliance policy in repo (`docs/hap-policy.md`) stating:
  - targeted HAP spec version/date,
  - transport scope (IP-only or IP+BLE),
  - required test suites and release gate.

### Phase 2 — Protocol and discovery alignment

- Review and update mDNS TXT payload generation:
  - `pv`, `ff`, `sf`, `c#`, `s#`, `ci`, `id`, `md`, `sh` handling.
- Keep transport flag handling configurable per build target and document approved values/profile mappings.
- If required by your target profile, enable/test IPv6 advertisement and resolution paths.

### Phase 3 — Security and crypto baseline

- Confirm all cryptographic primitives/parameters in `src/crypto.c` match latest HAP requirements.
- Pin dependency versions and publish rationale for:
  - `wolfssl/wolfssl` version,
  - ESP-IDF range,
  - mDNS component version.
- Add negative tests for pairing, verification, and encrypted transport failures.

### Phase 4 — Accessory model correctness

- Validate every exposed service/characteristic pair against latest HomeKit definitions.
- Verify mandatory characteristic presence, permissions, and read/write/event semantics.
- Verify error/status mapping for all HAP endpoints and characteristic operations.

### Phase 5 — Compliance evidence (required for "on the letter")

- Run Apple-required validation tooling/workflows for your target program.
- Store machine-readable test reports and traceability links (CI artifacts and release notes).
- Block releases unless all mandatory suites pass.

### Phase 6 — Documentation and release gating

- Rewrite README sections that currently imply uncertified/non-commercial-only posture if targeting strict compliance.
- Add a release checklist item: "latest HAP conformance evidence attached".
- Add a changelog section documenting protocol-level compliance changes.

## Immediate code hotspots to prioritize

- `src/server.c` mDNS TXT generation and setup URI payload flags.
- `src/homekit_mdns.c` IPv6 compile-time behavior (custom stack path).
- `src/crypto.c` pairing/verification/encryption compatibility checks.
- `README.md` claims and onboarding flow language.

## Minimum exit criteria for claiming latest HAP compliance

- All implementation deltas required by the latest HAP spec applied.
- Transport/profile behavior explicitly documented and tested.
- Apple-required compliance/certification tests passed for target product class.
- Evidence archived and linked from release artifacts.



## Implemented in this repository update

- Added Kconfig controls for HAP mDNS protocol/version metadata and setup payload flags.
- Replaced hard-coded `ff` mDNS TXT value with a configurable value (`HOMEKIT_MDNS_FEATURE_FLAGS`).
- Added optional IPv6 enablement in the built-in mDNS stack via `HOMEKIT_MDNS_ENABLE_IPV6`.
- Updated README with explicit HAP alignment controls and compliance note.

These changes improve alignment readiness but do not, by themselves, prove formal latest-HAP certification without Apple validation artifacts.
