* Use Structure of Arrays for the IR.
* Build def-use chains during Pass 1.
* Map SM86 predicates directly to SPIR-V `OpTypeBool`.
* Expand predicated SM86 instructions immediately during Pass 1.
* Emit the unpredicated instruction first.
* Pre-generate a static SPIR-V header containing common types and constants.
* Emit SPIR-V words using pointer increments: `*cursor++ == word`.
* Do not emit SPIR-V forward declarations inline.
* Map Constant Buffers to SPIR-V `Uniform` blocks.
* Treat UGPRs as standard thread-local registers.
* Ignore warp-level subgroup broadcasts for UGPRs to increase translation speed.

The IR design itself will be implemented using
this [design document](https://github.com/pound-emu/ballistic/blob/main/docs/IR_DESIGN_DOC.md).