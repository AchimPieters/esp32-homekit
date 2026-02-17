
# Security Policy

## Supported Versions
Only latest commit is supported.

## Security Model
This project implements:

- SRP authenticated pairing
- ChaCha20‑Poly1305 encrypted sessions
- Device‑unique key derivation
- Encrypted persistent storage
- Pairing rate limiting
- Permission enforcement

## Threat Model
Protects against:

- passive network attackers
- active MITM attempts
- replay attacks
- brute force pairing
- credential extraction from flash

Does NOT protect against:

- physical invasive attacks
- voltage glitching
- hardware side-channel attacks

## Reporting Vulnerabilities
Report privately to maintainer.
Do NOT open public issues for security bugs.
