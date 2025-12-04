# Core Principles

### 1. Performance First

Performance is the top priority in all aspects of development. Always ask yourself: "Is the CPU wasting cycles running
this code? If so, how do I fix it?".

### 2. Memory Management

- **Heap Allocation Functions Ban**: Using memory allocation functions like malloc(), free(), new, and delete is prohibited. Use our custom memory allocator in `src/host/memory/arena.h`. Allocating memory should only be done in `main.c` during initialization, otherwise your code will be rejected.

### 3. Safety First

1. **Barr C**: See the Barr C Style Guide PDF file in the root directory for making Pound's code as safe as possible.
2. **Error Handling**: Every return code from a function must be checked and handled appropriately.
3. **Assertions**: Use assertions to guarantee behavior whenever possible. Watch this video for an explanation:
   https://youtube.com/shorts/M-VU0fLjIUU
4. **Static Analysis**: Use the SonarQube static analyzer to catch potential bugs early in development.

### 4. Documentation

Document every struct and function throughout our codebase. This is a tedious task, but it will be greatly appreciated
by current and future programmers working on this project.

## Style Conventions

See the Barr C style guide for reference. Use clang-format at the root directory to create Barr C compliant code. Here are some non Barr C specific rules:

1. **Constant First in Equality Tests**:
   ```c
   // Non-compliant
   if (var == constant)
   if (pointer == NULL)
   
   // Compliant
   if (constant == var)
   if (NULL == pointer)
   ```
