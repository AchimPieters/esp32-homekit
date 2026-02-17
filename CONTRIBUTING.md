
# Contributing Guide

## Requirements
All contributions must:

- compile without warnings
- not increase heap allocations
- not add blocking calls
- follow existing style

## Pull Requests
Must include:

- explanation
- reason
- performance impact
- memory impact

## Forbidden Changes
PR will be rejected if it:

- increases binary size unnecessarily
- adds dynamic allocations in hot path
- weakens crypto
