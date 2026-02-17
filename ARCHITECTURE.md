
# Architecture Overview

## Layer Model

Application
↓
HAP Protocol Core
↓
Transport Abstraction
↓
Network Stack

## Core Principles
- deterministic state machine
- minimal heap allocation
- constant-time crypto operations
- modular isolation of subsystems

## Modules

### Protocol
Handles:
- pairing
- verification
- characteristics
- events

### Crypto
Encapsulates all cryptographic primitives.

### Storage
Provides encrypted persistence layer.

### Transport
Abstracts socket I/O.

### Parser
Streaming HTTP parser.

## Design Goals
- small binary size
- predictable timing
- auditability
- portability
