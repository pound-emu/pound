# Tiered JIT Compilation
Pound's current goal is to create a Task-Based Parallel JIT with Work-Stealing. This should hopefully be faster than Dynarmic's multithreading model. Wish me luck.

## Roadmap:
 [ ] Interpreter: Runs code immediately while the JIT compiles a faster version in the background.

 [ ] Single Worker Pipeline Model: Basic asynchronous compilation. 

 [ ] Task-Based System: Final Implementation.
