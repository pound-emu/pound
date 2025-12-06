# Tiered JIT Compilation
Pound's current goal is to create a Task-Based Parallel JIT with Work-Stealing. This should hopefully be faster than Dynarmic's multithreading model. Wish me luck.

## Roadmap:
 [ ] Interpreter: Runs code immediately while the JIT compiles a faster version in the background. I will be using LuaJIT's and Dolphin Emulator's Interpreter to guide me so that this wont take years to implement.

    1. [x] Translator: Convert raw arm32 binary code into Intermediate Representation.

    2. [ ] Register-Based Execution Engine: Run the converted binary code.

 [ ] Single Worker Pipeline Model: Basic asynchronous compilation. 

 [ ] Task-Based System: Final Implementation.
