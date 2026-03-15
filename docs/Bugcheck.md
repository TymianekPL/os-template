# Kernel bug check reference

`dbg::KeBugCheck` is the kernel's unrecoverable error handler. It fires when an exception cannot be recovered from, clears the screen to `#1E1E9E`, prints a structured crash dump, exchanges the video buffer, and halts all processors.

---

## `BugCheckInfo` structure

```cpp
struct BugCheckInfo {
    uint64_t       code;         // bug check code constant (see tables below)
    uint64_t       param1;       // meaning is code-specific (see tables below)
    uint64_t       param2;
    uint64_t       param3;
    uint64_t       param4;
    const char8_t* description;  // short human-readable cause string
    const char8_t* details;      // one-line developer hint
};
```

---

## Bug check codes

All codes are defined as `inline constexpr uint64_t` in `bugcheck.h`.

| Constant | Value | Raised by |
|---|---|---|
| `ErrorDivideError` | `0x01` | x86 `#DE`, ARM64 FP divide-by-zero |
| `ErrorDebugException` | `0x02` | x86 `#DB`, ARM64 breakpoint / watchpoint / step |
| `ErrorNMI` | `0x03` | x86 NMI vector |
| `ErrorBreakpoint` | `0x04` | x86 `#BP` (`INT3`), ARM64 `BRK`/`BKPT` |
| `ErrorOverflow` | `0x05` | x86 `#OF` (`INTO`) |
| `ErrorBoundRangeExceeded` | `0x06` | x86 `#BR` (`BOUND`) |
| `ErrorInvalidOpcode` | `0x07` | x86 `#UD`, ARM64 illegal state / AArch32 trap |
| `ErrorDeviceNotAvailable` | `0x08` | x86 `#NM` (FPU/SSE), ARM64 FP/SIMD/SVE access trap |
| `ErrorDoubleFault` | `0x09` | x86 `#DF` |
| `ErrorInvalidTSS` | `0x0A` | x86 `#TS` |
| `ErrorSegmentNotPresent` | `0x0B` | x86 `#NP` |
| `ErrorStackSegmentFault` | `0x0C` | x86 `#SS` |
| `ErrorGeneralProtectionFault` | `0x0D` | x86 `#GP`, ARM64 SVC-from-EL1 / MSR trap / ERET-PAC |
| `ErrorPageFaultInNonPagedArea` | `0x0E` | x86 `#PF` (non-fetch), ARM64 data abort |
| `ErrorPageFaultInPageProtectedArea` | `0x0F` | x86 `#PF` (instruction fetch) |
| `ErrorFloatingPointError` | `0x10` | x86 `#MF` (x87), ARM64 FP exception |
| `ErrorAlignmentCheck` | `0x11` | x86 `#AC` |
| `ErrorMachineCheck` | `0x12` | x86 `#MC`, ARM64 SError |
| `ErrorSIMDFloatingPointException` | `0x13` | x86 `#XM` (SSE/AVX) |
| `ErrorVirtualizationException` | `0x14` | x86 `#VE` (EPT) |
| `ErrorControlProtectionException` | `0x15` | x86 `#CP` (CET), ARM64 BTI |
| `ErrorSynchronousException` | `0x20` | ARM64 generic synchronous exception |
| `ErrorDataAbort` | `0x21` | ARM64 data abort (EC `0x24`/`0x25`) |
| `ErrorInstructionAbort` | `0x22` | ARM64 instruction abort (EC `0x20`/`0x21`) |
| `ErrorSPAlignmentFault` | `0x23` | ARM64 SP alignment fault (EC `0x26`) |
| `ErrorPCAlignmentFault` | `0x24` | ARM64 PC alignment fault (EC `0x22`) |
| `ErrorUnknownException` | `0xFF` | Any unrecognised vector or EC value |

---

## x86-64 exception vectors

`TranslateBugCheck` switches on `InterruptFrame::vector` (the IDT index).

| Vector | Mnemonic | Code constant | param1 | param2 | param3 | param4 |
|---|---|---|---|---|---|---|
| `0x00` | `#DE` | `ErrorDivideError` | RIP | - | - | - |
| `0x01` | `#DB` | `ErrorDebugException` | RIP | - | - | - |
| `0x02` | NMI | `ErrorNMI` | RIP | - | - | - |
| `0x03` | `#BP` | `ErrorBreakpoint` | RIP | - | - | - |
| `0x04` | `#OF` | `ErrorOverflow` | RIP | - | - | - |
| `0x05` | `#BR` | `ErrorBoundRangeExceeded` | RIP | - | - | - |
| `0x06` | `#UD` | `ErrorInvalidOpcode` | RIP | - | - | - |
| `0x07` | `#NM` | `ErrorDeviceNotAvailable` | RIP | - | - | - |
| `0x08` | `#DF` | `ErrorDoubleFault` | error code (always 0) | RSP | RIP | - |
| `0x0A` | `#TS` | `ErrorInvalidTSS` | selector | RIP | - | - |
| `0x0B` | `#NP` | `ErrorSegmentNotPresent` | selector | RIP | - | - |
| `0x0C` | `#SS` | `ErrorStackSegmentFault` | error code | RSP | RIP | - |
| `0x0D` | `#GP` | `ErrorGeneralProtectionFault` | error code | RIP | RSP | - |
| `0x0E` | `#PF` | `ErrorPageFault*` | faulting VA | error code | RIP | RSP |
| `0x10` | `#MF` | `ErrorFloatingPointError` | RIP | - | - | - |
| `0x11` | `#AC` | `ErrorAlignmentCheck` | faulting VA | RIP | error code | - |
| `0x12` | `#MC` | `ErrorMachineCheck` | RIP | - | - | - |
| `0x13` | `#XM` | `ErrorSIMDFloatingPointException` | RIP | - | - | - |
| `0x14` | `#VE` | `ErrorVirtualizationException` | RIP | - | - | - |
| `0x15` | `#CP` | `ErrorControlProtectionException` | error code | RIP | - | - |
| other | - | `ErrorUnknownException` | vector | error code | RIP | - |

### `#PF` error code bits (param2)

| Bit | Name | Meaning when set |
|---|---|---|
| 0 | P | Page was present (protection violation, not absence) |
| 1 | W | Access was a write |
| 2 | U | Access was from user mode (ring 3) |
| 3 | RSVD | Reserved PTE bit was set - PTE is corrupt |
| 4 | I | Access was an instruction fetch |
| 5 | PK | Protection-key violation |
| 6 | SS | Shadow-stack access (CET) |

The code is set to `ErrorPageFaultInPageProtectedArea` when bit 4 (I) is set, `ErrorPageFaultInNonPagedArea` otherwise. When `param1 < 0x10000`, the details string flags a likely null pointer dereference.

### x86-64 saved register frame layout

The `InterruptFrame` struct is pushed in this order by the exception entry stubs (high address → low):

```
ss, rsp, rflags, cs, rip, errorCode, vector,
rax, rbx, rcx, rdx, rsi, rdi, rbp,
r8, r9, r10, r11, r12, r13, r14, r15
```

---

## ARM64 exception classes (EC)

`TranslateBugCheck` switches on `ESR_EL1[31:26]` (the EC field). The full `ESR_EL1` value is always preserved in `InterruptFrame::esr`.

| EC | Description | Code constant | param1 | param2 | param3 | param4 |
|---|---|---|---|---|---|---|
| `0x00` | Unknown | `ErrorUnknownException` | ESR | LR | - | - |
| `0x01` | WFI/WFE trap | `ErrorSynchronousException` | ESR | LR | - | - |
| `0x03`–`0x06` | AArch32 CP14/CP15 trap | `ErrorInvalidOpcode` | ESR | LR | - | - |
| `0x07` | FP/SIMD/SVE access trap | `ErrorDeviceNotAvailable` | ESR | LR | - | - |
| `0x0C` | FP exception (AArch32) | `ErrorFloatingPointError` | ESR | LR | - | - |
| `0x0E` | Illegal execution state | `ErrorInvalidOpcode` | ESR | LR | - | - |
| `0x11` | SVC (AArch32) | `ErrorGeneralProtectionFault` | SVC imm | LR | - | - |
| `0x15` | SVC (AArch64) | `ErrorGeneralProtectionFault` | SVC imm | LR | - | - |
| `0x18` | MSR/MRS/system instruction trap | `ErrorGeneralProtectionFault` | ESR | LR | - | - |
| `0x19` | SVE access trap | `ErrorDeviceNotAvailable` | ESR | LR | - | - |
| `0x1A` | ERET/ERETAA/ERETAB trap | `ErrorGeneralProtectionFault` | ESR | LR | SP | - |
| `0x1C` | FP exception (AArch64) | `ErrorFloatingPointError` | ESR | LR | - | - |
| `0x1F` | Implementation-defined | `ErrorUnknownException` | ESR | LR | - | - |
| `0x20` | Instruction abort (lower EL) | `ErrorInstructionAbort` | FAR | ESR | LR | - |
| `0x21` | Instruction abort (same EL) | `ErrorInstructionAbort` | FAR | ESR | LR | - |
| `0x22` | PC alignment fault | `ErrorPCAlignmentFault` | FAR | LR | - | - |
| `0x24` | Data abort (lower EL) | `ErrorDataAbort` | FAR | ESR | LR | SP |
| `0x25` | Data abort (same EL) | `ErrorDataAbort` | FAR | ESR | LR | SP |
| `0x26` | SP alignment fault | `ErrorSPAlignmentFault` | SP | LR | - | - |
| `0x2C` | BTI branch target failure | `ErrorControlProtectionException` | FAR | ESR | LR | - |
| `0x2F` | SError interrupt | `ErrorMachineCheck` | ESR | LR | - | - |
| `0x30` | Breakpoint (lower EL) | `ErrorDebugException` | FAR | ESR | - | - |
| `0x31` | Breakpoint (same EL) | `ErrorDebugException` | FAR | ESR | - | - |
| `0x32` | Software step (lower EL) | `ErrorDebugException` | FAR | ESR | - | - |
| `0x33` | Software step (same EL) | `ErrorDebugException` | FAR | ESR | - | - |
| `0x34` | Watchpoint (lower EL) | `ErrorDebugException` | FAR (watch addr) | ESR | LR | - |
| `0x35` | Watchpoint (same EL) | `ErrorDebugException` | FAR (watch addr) | ESR | LR | - |
| `0x38` | BKPT (AArch32) | `ErrorBreakpoint` | ISS imm16 | LR | - | - |
| `0x3C` | BRK (AArch64) | `ErrorBreakpoint` | ISS imm16 | LR | - | - |
| `0x3D` | Vector catch (AArch32) | `ErrorSynchronousException` | ESR | LR | - | - |
| other | Unrecognised EC | `ErrorUnknownException` | ESR | FAR | LR | - |

### Data abort sub-classification (EC `0x24`/`0x25`)

The data abort handler further classifies by `DFSC` (`ESR_EL1[5:0]`) and the `WnR` bit (`ESR_EL1[6]`).

| DFSC range | Category | WnR=0 description | WnR=1 description |
|---|---|---|---|
| `0x04`–`0x07` | Translation fault | Read from unmapped page | Write to unmapped page |
| `0x08`–`0x0B` | Access flag fault | Access flag fault (AF=0) | Access flag fault (AF=0) |
| `0x0C`–`0x0F` | Permission fault | Read permission fault | Write to read-only page |
| `0x10` | Synchronous external abort | Bus/ECC error | Bus/ECC error |
| `0x21` | Alignment fault | Misaligned access | Misaligned access |
| `0x30` | TLB conflict | TLB conflict abort | TLB conflict abort |

When `FAR_EL1 < 0x10000`, the details string overrides to flag a likely null pointer dereference regardless of DFSC.

### DFSC / IFSC values

| Value | Meaning |
|---|---|
| `0x04` | Translation fault, level 0 |
| `0x05` | Translation fault, level 1 |
| `0x06` | Translation fault, level 2 |
| `0x07` | Translation fault, level 3 (leaf PTE not present) |
| `0x09` | Access flag fault, level 1 |
| `0x0A` | Access flag fault, level 2 |
| `0x0B` | Access flag fault, level 3 |
| `0x0D` | Permission fault, level 1 |
| `0x0E` | Permission fault, level 2 |
| `0x0F` | Permission fault, level 3 |
| `0x10` | Synchronous external abort |
| `0x21` | Alignment fault |
| `0x30` | TLB conflict abort |

### ARM64 saved register frame layout

```cpp
struct InterruptFrame {           // offset  (×8 bytes)
    uint64_t x0–x7;              //  0–7
    uint64_t x8–x15;             //  8–15
    uint64_t x16–x18;            // 16–18
    uint64_t x19–x28;            // 19–28
    uint64_t fp;   // x29        // 29
    uint64_t lr;   // x30        // 30
    uint64_t vector;              // 31
    uint64_t esr;                 // 32
    uint64_t far;                 // 33
    uint64_t svc;                 // 34  (valid only when vector == 0x15)
    uint64_t sp;                  // 35
};
// static_assert(sizeof(InterruptFrame) == 36 * 8);
```

---

## Screen output format

```
*** KERNEL BUGCHECK ***
An unrecoverable error has occurred. The system has been halted.

Cause:  <description>
Detail: <details>

Bug Check Code: <code (hex, 16 digits)>
Parameters:
* <param1>
* <param2>
* <param3>
* <param4>

Register Dump:
<platform register dump>
```

The background colour is `#1E1E9E`. All text is drawn white (`#FFFFFF`) at 8×10 pixels per character via `VidDrawChar`. `VidExchangeBuffers` is called once at the end so the back buffer is flipped to screen before the halt loop.

---
