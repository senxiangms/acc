# Programming model: Architecture for AI Computing 

This document defines how host software expresses work for a device that resembles Huawei Ascend-style NPUs: separate host and device address spaces, command queues, DMA transfers, and *device kernels* described by a fixed ABI (not a full compiler stack in this repo).

## 1. Execution model

- **Host**: runs the control program (CPU). It allocates buffers, enqueues copies and kernel launches, and synchronizes.
- **Device**: executes **kernels** from a **command queue**. Kernels are identified by a small **kernel descriptor** (entry offset, grid dimensions, argument table).
- **Asynchrony**: submissions return immediately; `aac_sync(queue)` waits for all prior work on that queue.

## 2. Memory model

| Space        | Visibility | Typical use                          |
|-------------|------------|--------------------------------------|
| Host memory | CPU        | Input/output tensors, staging        |
| Device memory | NPU only | Weights, activations, scratch        |

The host **never** dereferences device pointers. It only passes device-side addresses (handles or offsets) to launch/copy APIs. The driver maps device memory and programs DMA engines (real hardware) or emulates behavior (bring-up).

## 3. Objects

- **Device** (`aac_device_t`): one accelerator instance; opens `/dev/aacN`.
- **Queue** (`aac_queue_t`): ordered stream of DMA and kernel commands.
- **Buffer** (`aac_buffer_t`): sized allocation; may be host-visible (pinned) or device-only, depending on flags.

## 4. Kernel ABI (device side)

Kernels are *not* compiled here; you supply:

- **Code blob**: device instruction bytes (or firmware-loaded segment).
- **Entry offset**: where execution starts in that blob.
- **Grid**: 3D launch grid `(gx, gy, gz)`; each **block** has `(bx, by, bz)` threads (logical; mapping to real SIMD/warps is hardware-specific).
- **Args**: table of `(type, value)` — e.g. `U64` device addresses, `U32` scalars.

The **driver** validates bounds and enqueues a **launch packet** the device firmware interprets. On real silicon, that packet matches your hardware’s doorbell / ring buffer layout.

## 5. Error and ordering

- Commands on a single queue are **strictly ordered**.
- Multiple queues are **unordered** relative to each other unless you add explicit cross-queue sync (future extension).

## 6. Relation to Ascend-style stacks

Commercial stacks (e.g. CANN) add graph capture, tiling, fusion, and proprietary IR. This repo keeps the same *conceptual* split—host runtime, device memory, streams, kernels—so you can later swap the IOCTL packet format for real ASCEND-like firmware without rewriting the host API shape.
