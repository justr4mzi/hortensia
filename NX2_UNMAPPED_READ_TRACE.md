# Unmapped device-read attribution

Set `EDEN_NX2_UNMAPPED_READ_TRACE=1` before starting Orchid to enable this diagnostic. It is off by default. The first unmapped device read with a device request size of 69,632 bytes and the first with a size of 20,480 bytes produce at most two additional records per process, independently of the existing unmapped-read warning limit. Existing warnings and zero filling are unchanged.

Each record includes the immediate device-read call site, request address and size, and the faulting device-address chunk. When available it also includes the enclosing GPU read, the translated GPU chunk, and the buffer synchronization caller and range. Boolean fields indicate which contexts are present; an enclosing GPU read is not necessarily the origin of a nested synchronization read. Worker origin, shader stage, and dispatch are unknown from this record. Context collection does not allocate or read guest memory. The existing logging backend may allocate while formatting the record. This diagnostic does not identify the cause of a device loss or change GPU behavior.

For a CPU-only integration check after building the `tests` target, run `python3 src/tests/video_core/unmapped_trace_integration.py <path-to-tests>`. The script runs separate processes with tracing off and on and checks both warning limits and the actual log fields.
