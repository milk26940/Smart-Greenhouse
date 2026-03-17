# Serial CLI Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a local Python serial debugging CLI with `list`, `monitor`, `send`, and `log` commands for STM32 development workflows.

**Architecture:** Keep serial operations in a reusable library module and keep command parsing in a thin CLI entrypoint. Use `unittest` with mocks so behavior can be verified without requiring physical hardware.

**Tech Stack:** Python 3, `pyserial`, `argparse`, `unittest`

---

## Chunk 1: Tests First

### Task 1: Add library tests

**Files:**
- Create: `tests/test_serial_lib.py`
- Test: `tests/test_serial_lib.py`

- [ ] **Step 1: Write failing tests for port listing, send, and stream helpers**
- [ ] **Step 2: Run `python -m unittest tests.test_serial_lib -v` and verify failure**
- [ ] **Step 3: Implement minimal library code in `tools/serial_lib.py`**
- [ ] **Step 4: Run `python -m unittest tests.test_serial_lib -v` and verify pass**

### Task 2: Add CLI tests

**Files:**
- Create: `tests/test_serial_cli.py`
- Test: `tests/test_serial_cli.py`

- [ ] **Step 1: Write failing tests for `list`, `send`, and `log` command dispatch**
- [ ] **Step 2: Run `python -m unittest tests.test_serial_cli -v` and verify failure**
- [ ] **Step 3: Implement minimal CLI code in `tools/serial_cli.py`**
- [ ] **Step 4: Run `python -m unittest tests.test_serial_cli -v` and verify pass**

## Chunk 2: Implementation

### Task 3: Complete streaming and logging behavior

**Files:**
- Modify: `tools/serial_lib.py`
- Modify: `tools/serial_cli.py`
- Modify: `tests/test_serial_lib.py`
- Modify: `tests/test_serial_cli.py`

- [ ] **Step 1: Add failing tests for monitor/log loops and graceful shutdown**
- [ ] **Step 2: Run focused unittest commands and verify failure**
- [ ] **Step 3: Implement streaming output, file logging, and interrupt-safe cleanup**
- [ ] **Step 4: Run focused unittest commands and verify pass**

### Task 4: Add usage documentation

**Files:**
- Create: `tools/README-serial-cli.md`

- [ ] **Step 1: Document installation and command examples**
- [ ] **Step 2: Review docs against actual CLI behavior**

## Chunk 3: Verification

### Task 5: Verify end to end

**Files:**
- Modify: `tools/README-serial-cli.md`

- [ ] **Step 1: Run `python -m unittest discover -s tests -v`**
- [ ] **Step 2: Run `python tools/serial_cli.py --help`**
- [ ] **Step 3: Run `python tools/serial_cli.py list` if `pyserial` is installed**
- [ ] **Step 4: Record any environment limits in the final summary**
