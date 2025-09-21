# Data-Oriented Design Fundamentals

This document outlines the core principles and guidelines for contributing to our project, which has been rewritten
using a data-oriented design (DoD) approach.

## Why Data-Oriented Design?

The source code is now written in a data-oriented style instead of an object-oriented one. This change was made because
traditional OOP can be CPU cache-unfriendly, leading to slower performance compared to DoD principles.

While Yuzu's entire codebase is object-oriented, we believe that applying DoD from the very beginning could
significantly improve its speed and efficiency.

## Key Resources

To learn more about data-oriented design fundamentals, please refer to this invaluable resource:

https://github.com/dbartolini/data-oriented-design

This guide should be treated as a fundamental reference when working on our codebase.

## Core Principles

### 1. Performance First

Performance is the top priority in all aspects of development. Always ask yourself: "Is the CPU wasting cycles running
this code? If so, how do I fix it?" The data-oriented design resource above contains answers to these questions.

### 2. Memory Management

- **Heap Allocation Ban**: Using memory allocation functions like malloc(), free(), new, and delete is prohibited.
- **Stack Preference**: Keep everything on the stack whenever possible.
- **Last Resort**: If you must allocate memory, use our custom memory allocator in `core/memory/arena.h`. This should be
  your last resort only.

The reason for these strict rules is that heap allocations can introduce undefined behavior issues into our codebase.

### 3. Safety First

1. **Error Handling**: Every return code from a function must be checked and handled appropriately.
2. **Assertions**: Use assertions to guarantee behavior whenever possible. Watch this video for an explanation:
   https://youtube.com/shorts/M-VU0fLjIUU
3. **Static Analysis**: Use the SonarQube static analyzer to catch potential bugs early in development.

### 4. Documentation

Document every struct and function throughout our codebase. This is a tedious task, but it will be greatly appreciated
by current and future programmers working on this project.

## Style Conventions

Refer to `main.cpp` and the kvm folder for examples of proper code styling. Here are some specific rules:

1. **Constant First in Equality Tests**:
   ```c
   // Non-compliant
   if (var == constant)
   if (pointer == NULL)
   
   // Compliant
   if (constant == var)
   if (NULL == pointer)
   ```
2. **Todo Comments**: Use the following format:
   ```c
   // Todo(<Username>:<section>): ...

   Where <section> is one of:

   - cpu
   - gpu
   - gui
   - memory
   - filesystem

    For example:
    // Todo(GloriousTaco:memory): Create a custom allocator.
   ```

