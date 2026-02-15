# pilot-link Development Plan - Code Analysis & Improvement Report

> Analysis Date: 2026-02-14
> Last Updated: 2026-02-14
> Scope: Full codebase review of pilot-link 0.13.0

---

## Progress Overview

| Phase | Status | Description |
|-------|--------|-------------|
| Phase 1 | **DONE** | Critical Security Fixes |
| Phase 2 | **DONE** | High Priority Fixes |
| Phase 3 | **DONE** | Build System & Language Binding Modernization |
| Phase 4 | **DONE** | Feature Completion |
| Phase 5 | **PARTIAL** | Test & Quality Infrastructure |

Build verification: `autoreconf -fi && ./configure && make && make check` — **PASS** (3/3 tests)

---

## Target Environment

| Component | Target Version | Migration Required |
|-----------|---------------|-------------------|
| **Python** | **3.10** | YES - CRITICAL |
| **Java** | **OpenJDK 21** (LTS) | YES - MAJOR |
| **Perl** | **5.34** | YES - MODERATE |
| **Tcl** | **8.6** | YES - MODERATE |
| **Autoconf** | 2.71+ | YES |
| **Automake** | 1.16+ | YES |
| **C Standard** | C11 (gcc/clang) | YES |

---

## Phase 1: Critical Security Fixes — DONE

All items completed. Summary of changes:

| Item | Files Modified | Change |
|------|---------------|--------|
| `strcpy()`/`sprintf()` → bounded alternatives | `dlp.c`, `mail.c`, `money.c`, `versamail.c`, `location.c`, `debug.c`, userland tools (~40 files) | ~120 replacements with `memcpy()`, `snprintf()`, length-checked copies |
| Pointer-size bug | `libpisync/sync.c:98` | `sizeof(precord->buffer)` → `precord->len` |
| DLP response bounds checking | `dlp.c:518-550` | Added buffer length validation before argument extraction |
| VFS mount double-write | `dlp.c:4570-4589` | Fixed duplicate write at offset 4 |
| `pi_dumpline()` buffer overflow | `debug.c:175-212` | Fixed 256-byte buffer overflow with bounded writes |

---

## Phase 2: High Priority Fixes — DONE

| Item | Files Modified | Change |
|------|---------------|--------|
| `malloc()`/`realloc()` NULL checks | `pilot-xfer.c` (5 sites), `pi-buffer.c`, others (~30 sites) | Added NULL checks, temp pointer pattern for realloc |
| `fopen()`/`open()` return validation | `pilot-file.c`, `pilot-foto.c`, `pilot-read-notepad.c`, others (~15 sites) | Added error checks |
| Signal handler deadlock | `socket.c` | Replaced mutex in handler with `volatile sig_atomic_t` flag + `pi_process_tickles()` in main thread |
| `strncpy()` NULL termination | `socket.c` (5), `dlp.c` (2), `pilot-dlpsh.c`, `pilot-install-netsync.c` (3), `contact.c` | Added explicit `\0` termination; binary data changed to `memcpy()` |
| Memory leaks in error paths | `pilot-xfer.c`, `userland.c` | Fixed fd leak on negative sd, realloc chains |

---

## Phase 3: Build System & Language Binding Modernization — DONE

### 3-A. Build System

| Item | Change |
|------|--------|
| `configure.ac` modernization | `AC_PREREQ([2.71])`, `AC_CONFIG_HEADERS`, `AC_CANONICAL_HOST`, `LT_INIT`, `AC_*_IFELSE` forms, removed `AC_PROG_F77`, fixed nested AC_CHECK macro shell syntax error |
| libpng compatibility | Removed deprecated `png_voidp_NULL`/`png_error_ptr_NULL` in 4 files → plain `NULL` |

### 3-B. Python 3.10 Migration

| Item | Change |
|------|--------|
| SWIG interfaces | `PyInt_*` → `PyLong_*`, `PyString_*` → `PyUnicode_*`/`PyBytes_*` in `.i` files |
| `pisock_wrap.c` | Regenerated with SWIG 4.x |
| Test code | `print` → `print()`, `.has_key()` → `in`, `distutils` → `setuptools` |
| `m4/python.m4` | Version extraction fixed for 3.10+ |

### 3-C. Java 21 Migration

| Item | Change |
|------|--------|
| `build.xml` | `target="1.5"` → `release="21"`, project name typo fixed |
| `Makefile.am` | ARCH condition inversion fixed, JNI install path modernized |
| Code | Parameter typo `dbnmae` → `dbname` |

### 3-D. Perl 5.34 Migration

| Item | Change |
|------|--------|
| `configure.ac` | `PERL_MINVERSION` → `"5.034_000"` |
| `Makefile.PL.in` | `-DPERL_POLLUTE` removed |
| `Pilot.xs` | `newRV_noinc` compat macro removed, `PATCHLEVEL < 3` branch removed |

### 3-E. Tcl 8.6 Migration

| Item | Change |
|------|--------|
| `pitcl.c` | Pre-Tcl 8 code removed (`OldPisockGetHandleProc`, `PisockReadyProc`, `#if TCL_MAJOR_VERSION >= 8` guards) |
| `m4/tcl.m4` | Tcl 8.6 minimum version verification added |

---

## Phase 4: Feature Completion — DONE

| Item | Files Modified | Change |
|------|---------------|--------|
| PalmPix thumbnail reader | `palmpix.c` | Implemented 4bpp/8bpp greyscale thumbnail reader with RGB conversion |
| Linux USB `wait_for_device` | `linuxusb.c` | Implemented fd validation via `fcntl(F_GETFL)` |
| FreeBSD USB timeout | `freebsdusb.c` | Added `select()` with proper timeout handling |
| Bluetooth listen() validation | `bluetooth.c` | Added error check on `listen()`, use `backlog` parameter |
| Veo length validation | `veo.c` | Added `len < 25` input check, removed misleading FIXME |
| Notepad length fix | `notepad.c` | Added missing `buffer += dataLen` after memcpy |
| Timezone correction | `pi-file.c` | Implemented `gmtime_r` + `mktime` trick for both directions |
| address.c duplicate code | `address.c` | Removed duplicate `phoneLabels` copy loops (const UB) |
| pilot-read-todos Private/Delete | `pilot-read-todos.c` | Added `dlpRecAttrSecret`/`dlpRecAttrDeleted` output |
| pilot-install-netsync DNS | `pilot-install-netsync.c` | Auto-resolve IP↔hostname via `getnameinfo()`/`getaddrinfo()` |

---

## Phase 5: Test & Quality Infrastructure — PARTIAL

### Completed

| Item | Change |
|------|--------|
| `tests/Makefile.am` fixes | Removed duplicate `contactsdb-test`, removed ghost `contactsdb-jps` reference |
| `packers.c` fix | `main()` now returns error count (was always 0) |
| `buffer-test.c` | New unit test: pi_buffer new/free/append/expect/clear/append_buffer/stress (17 checks) |
| `time-test.c` | New unit test: Palm time roundtrip/ordering/delta (15 checks) |
| GitHub Actions CI | `.github/workflows/ci.yml` — gcc/clang matrix + minimal build config |
| `make check` | 3/3 tests PASS (packers, buffer-test, time-test) |

### Remaining

#### 28. Fuzz Testing for Protocol Parsers
- DLP response parsing (`dlp.c`)
- SLP packet parsing (`slp.c`)
- PADP frame parsing (`padp.c`)
- File format parsing (`pi-file.c`)
- Image format parsing (`palmpix.c`, `notepad.c`, `veo.c`)

#### 29. Additional Unit Tests
Modules without offline-runnable tests:

| Module | Missing Tests |
|--------|--------------|
| `libpisock/dlp.c` | Malformed response parsing, boundary conditions |
| `libpisock/slp.c` | SLP packet parsing edge cases |
| `libpisock/padp.c` | PADP fragmentation/reassembly |
| `libpisock/net.c` | Network protocol error handling |
| `libpisock/cmp.c` | CMP negotiation failures |
| `libpisock/money.c` | Money packer/unpacker |
| `libpisock/hinote.c` | Hi-Note packer/unpacker |
| `libpisock/notepad.c` | Notepad packer/unpacker |
| `libpisock/palmpix.c` | PalmPix image handling |
| `libpisock/veo.c` | Veo video handling |
| `libpisock/blob.c` | Blob data handling |
| `libpisync/sync.c` | Sync record merging, conflict resolution |

Note: Device-dependent tests (`calendardb-test`, `contactsdb-test`, `dlp-test`, `locationdb-test`, `versamail-test`, `vfs-test`) require a physical Palm device and remain as `noinst_PROGRAMS`.

#### 30. Binding-Specific Tests
- Python: `pytest` integration
- Perl: `Test::More` rewrite of `test.pl`
- Tcl: `tcltest` rewrite of `test.tcl`

#### 31. Error Return Standardization
Multiple functions in `libpisock/` return raw `-1` instead of `PI_ERR_*` constants. Scope: codebase-wide audit of public API return values.

#### 32. Feature Enhancements (from doc/TODO)

| Category | Item |
|----------|------|
| Enhancement | Consolidate `install-*` binaries into multi-call binary |
| Enhancement | Standard output formats (XML/CSV/iCal/vCard) for conduit data |
| Enhancement | `pilot-xfer -f` support for `FooDB.p{db|rc|qa}` |
| Enhancement | `pilot-xfer -f` pull by Creator ID |
| Enhancement | `pilot-xfer --daemon` mode |
| Minor | Add current date/time to `dlp_AddSyncLogEntry()` |
| Minor | `install-hinote` duplicate detection |
| Major | RPC code broken on big-endian machines |

#### 33. Cosmetic / Low Priority

| Item | Scope |
|------|-------|
| `$Id$` CVS markers removal | Codebase-wide |
| `.cvsignore` → `.gitignore` consolidation | ~10 files |
| Mixed tab/space indentation cleanup | Codebase-wide |
| Inconsistent function naming (camelCase vs snake_case) | Codebase-wide |

---

## Known Issues

| Issue | Location | Status |
|-------|----------|--------|
| `vfs-test.c:410`: `dlp_VFSFileSetDate` documented as FAILED | tests/ | Known bug, unfixed |
| `dlp_CallApplication`, `dlp_ResetSystem`, `dlp_ProcessRPC` excluded from tests | vfs-test.c:22-26 | Intentionally skipped (dangerous on device) |
| `pack_Veo()` / `pack_NotePad()` return 0 (stub) | veo.c, notepad.c | Stub — no Palm-side write support needed |
| `freebsdusb.c` `u_poll()` is a stub | freebsdusb.c:242-252 | Logs warning, returns success |
| Serial port reconfiguration blocked when connected | serial.c:649 | Design limitation |
| Contact blob unknown field at `data+2` | contact.c:231 | Always null in observed data |
| Thread safety silently disabled without pthreads | threadsafe.c:27-52 | By design (single-threaded fallback) |

---

## Language Binding Reference

### Python 3.10 Policy
- Legacy Python 2.x support fully removed
- SWIG interfaces use Python 3 C API (`PyLong_*`, `PyUnicode_*`, `PyMemoryView_*`)
- Build uses `setuptools` (not `distutils`)
- Tests use `pytest` or `unittest`

### Java 21 Policy
- OpenJDK 21 LTS target
- `build.xml` uses `release="21"`
- JNI library installed to standard `${libdir}/` path
- JPMS: runs as unnamed module (classpath mode)

### Perl 5.34 Policy
- Minimum version `5.034_000`
- No `PERL_POLLUTE`, no pre-5.004 compat macros
- `#undef dirty` hack retained (still needed for Perl/GCC macro collision)

### Tcl 8.6 Policy
- Minimum version 8.6
- Pre-Tcl 8 compat code removed
- Tcl_Obj API used throughout
- Currently 5 commands implemented (piOpen, dlpStatus, dlpOpen, dlpPackers, dlpGetRecord)
- Expansion to full DLP binding is a future task
