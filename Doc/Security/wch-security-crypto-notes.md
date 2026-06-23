# WCH Security, Crypto, CRC, And RNG Notes

This document extracts hardware crypto, CRC, and random-number guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future HAL metadata, boot/update validation, secure transport, and high-speed encrypted transfer templates.

Official EVT examples, RM, DS, security reviews, external protocol specifications, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/ecdc_crypto.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/ecdc_crypto.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/ecdc_crypto.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/crc_calculation.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/peripheral_api.md`

## Capability Matrix

| Capability | Families from source notes | Main constraints |
|---|---|---|
| CH56x ECDC | CH569 / CH56x RISC-V | AES-128/192/256 and SM4, ECB/CTR, single-register, self-DMA, and HSPI-linked DMA; RAMX placement and key initialization are required. |
| CH32H ECDC | CH32H417 | AES-128/192/256, ECB/CBC, RAM block encryption/decryption; 32-byte alignment and 128-bit block length units are required. |
| CH561/CH563 crypto | CH561/CH563 ARM7TDMI | No ECDC engine in source notes; use software crypto or choose CH569 when hardware encryption is required. |
| Hardware CRC | CH32H417 source notes; CH561/CH563 notes mention CRC availability to verify | Data integrity, firmware validation, and protocol checks; polynomial, seed, word order, and reset behavior must be explicit. |
| Hardware RNG | CH32F20x and CH32H417 source notes | Wait for data-ready flags; raw hardware output still needs security review and conditioning for cryptographic use. |

## Crypto Boundary Rules

- Keep encryption, integrity-only CRC, random generation, key storage, and protocol authentication as separate concepts.
- Hardware ECDC accelerates block ciphers; it does not by itself provide authentication, secure key storage, anti-rollback, or transport security.
- CRC is not a cryptographic signature or MAC. Use it for accidental-corruption checks, not attacker resistance.
- RNG output should be treated as entropy input. Cryptographic uses need conditioning, health checks, and a security review.
- Do not claim security properties from Markdown extraction alone; verify against datasheets, threat model, and product requirements.

## ECDC Rules

CH56x ECDC rules:

- Always initialize key and mode with `ECDC_Init()` or set the key with `ECDC_SetKey()` before encryption/decryption. Source pitfalls state the key register contains random values at boot.
- Select algorithm, block mode, key length, clock, execution mode, and endianness explicitly.
- Treat `ECDC_SelfDMA()` length as 32-bit words according to the source notes, not bytes.
- Place self-DMA buffers in RAMX (`0x20020000+` in the source recipe); normal RAM may not be accessible by ECDC DMA.
- ECB input must be a multiple of 16 bytes. CTR mode also needs explicit counter/nonce management and reload rules.
- For HSPI-linked DMA, record the data path direction: RAM-to-peripheral encrypt/decrypt or peripheral-to-RAM encrypt/decrypt.
- Keep ECDC clock configuration separate from system clock. Source notes mention 240 MHz or 160 MHz ECDC clock and a minimum system clock expectation.

CH32H ECDC rules:

- Enable the ECDC bus/peripheral clock and hardware clock before configuring transfers.
- Use aligned buffers; source notes use 32-byte alignment.
- Store source address, destination address, block count, algorithm, block mode, key length, IV, key, endian mode, and execution mode in metadata.
- Wait for completion flags such as RAM-to-RAM end before reading destination buffers and clear flags after use.
- Source notes list AES with ECB/CBC modes. Do not infer SM4, CTR, GCM, or authenticated modes without header/RM verification.

CH561/CH563 rules:

- Mark ECDC as unavailable. Do not generate CH569 ECDC calls for CH561/CH563.
- Software AES/SM4 may be used, but performance and memory impact must be considered.
- If encrypted Ethernet or eMMC transfer is required, CH569 is the hardware-accelerated family indicated by the source notes.

## CRC Rules

CRC is useful for boot, storage, and communications integrity checks only when both sides use the same profile.

Rules:

- Store full CRC profile: width, polynomial, initial value, input reflection, output reflection, final XOR, feed width, byte/word order, and reset behavior.
- Reset CRC state before starting a new independent calculation.
- Keep firmware-image CRC, packet CRC, storage-block CRC, and protocol-specific CRC as separate profiles.
- If using the CRC peripheral in multiple modules, define ownership or locking; a single hardware CRC state can be clobbered by unrelated code.
- For bootloader validation, pair CRC with image length, destination range, stack/reset-vector checks, and optional signature policy from `Doc/IAP/wch-iap-ota-notes.md`.
- Do not use CRC as proof of authenticity.

## RNG Rules

RNG is security-sensitive and should not be treated as a normal counter or PRNG helper.

Rules:

- Enable the correct bus/peripheral clock before enabling the RNG.
- Wait for data-ready flags such as `RNG_FLAG_DRDY` before reading.
- Handle startup delay and error/status flags where the target exposes them.
- Do not use modulo reduction for security-sensitive bounded random values because it can introduce bias; use rejection sampling when uniform distribution matters.
- Do not use raw RNG output directly for long-term cryptographic keys without conditioning, health checks, and security review.
- Store availability by chip. Source notes explicitly say CH32F20x has RNG and CH32F10x/CH32M030 do not.

## Integration With Other Topics

- `Doc/Storage/wch-storage-notes.md`: encrypted eMMC, HSPI ECDC, and external flash integrity need crypto/storage cross-metadata.
- `Doc/IO/wch-io-media-notes.md`: high-throughput DMA streams need buffer ownership and alignment rules before adding encryption.
- `Doc/IAP/wch-iap-ota-notes.md`: firmware update validation may use CRC for accidental corruption and signatures/MACs for authenticity.
- `Doc/System/wch-system-analog-power-notes.md`: RNG, CRC, and power/clock gating rules should agree with system clock and low-power policy.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| ECDC key not initialized | Random or unrecoverable ciphertext | Always initialize key/counter before use. |
| ECDC buffer in wrong RAM | DMA transfer fails or corrupts data | Use family-required DMA-accessible RAM such as CH56x RAMX. |
| ECDC length unit misread | Partial transfer or overflow | Record whether length is words, bytes, or 128-bit blocks. |
| CTR counter reused | Confidentiality failure | Treat counter/nonce uniqueness as part of protocol metadata. |
| ECB used for structured data | Pattern leakage | Prefer a reviewed mode/padding/protocol; do not default to ECB for product security. |
| CRC used as authentication | Attacker can modify data and recompute CRC | Use MAC/signature for authenticity. |
| CRC profile incomplete | Mismatched values between systems | Store polynomial, seed, bit order, final XOR, and feed order. |
| RNG read before ready | Undefined or repeated values | Wait for ready and handle errors. |
| RNG modulo reduction for secrets | Biased bounded values | Use rejection sampling for uniform secure ranges. |

## Metadata Checklist

Future security/crypto templates should represent these fields explicitly:

- Crypto capability: none, software-only, CH56x ECDC, CH32H ECDC, or other verified hardware block.
- Algorithm, block mode, key length, IV/counter/nonce policy, endianness, padding, authentication policy, and key lifecycle.
- Execution path: single-register, RAM-to-RAM DMA, self-DMA, peripheral-linked DMA, or software.
- Buffer requirements: address range, alignment, length unit, cache/coherency, ownership, and in-place/out-of-place behavior.
- CRC profile: width, polynomial, seed, reflection, final XOR, feed width/order, reset, and owner/lock policy.
- RNG profile: availability, clock, ready/error flags, conditioning, health checks, and approved cryptographic usage.
- Verification source: exact EVT example, RM/DS chapter, header definitions, test vectors, and security review status.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, EVT source trees, RM, DS, official security documentation, known-answer test vectors, side-channel behavior, or a product threat model in this pass.
- Treat API names, mode availability, length units, clock limits, RAM requirements, RNG quality, CRC defaults, and security claims as preliminary until checked against the exact target chip, official examples, and security requirements.
