## What's new

First build of the `mario-tennis-fever` branch, based on the nx2 branch at ef7160c.

### Mario Tennis Fever
- Fixed a startup crash in audio: circular buffer sinks were cleaned up as if they had an upsampler (1949e30)
- I2F.F32.S16 and I2F.F32.U8 shaders are now lowered instead of rejected (1949e30)
- New per-game defaults: low GPU accuracy (enables the GPU thread split, roughly 30–60 fps in a match) and balanced fences so the characters' conditional rendering still sees occlusion results. Your own per-game settings always win (d9e9adf)

### GPU / Video core
- Query reports are flushed before native and split-front MME DMA reads (58bb176, 9c2ce7f)
- DMA pusher checks GPU-written pages before draining the split back end (821d891)
- Layered cross-format texture copies are split into per-layer copies (4728611)
- Surface and texture descriptor buffers are now separate (71575d2)

### Ampere shader recompiler
- Many new instruction forms lowered: ISETP.EX, REDUX.MIN, SHF with constant source, LEA / LEA.HI / LEA.HI.X variants, IMAD.WIDE, IMAD.X, IADD3 with carry predicates, F2I.U64, I2F from 64-bit unsigned pairs, IDP.4A.S8.S8, TXQ.B, partial SUST.P masks, uniform bindless TEX with LOD/bias, X8Z24 formats and more
- Calls inside the captured library region are lowered (c434c02)
- Geometry exit analysis for F2I, LEA, LDC, LOP3, AST and exit handlers (54972d9)

### Kernel
- A same-core thread started before an IPC call now gets to run its entry code (adcc21f)

### Docs
- Fire Emblem: Fortune's Weave bring-up notes on Linux and notes on the remaining compute instructions
