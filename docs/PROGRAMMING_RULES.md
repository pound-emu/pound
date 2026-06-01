# Compiler Aliasing Fear

Pointer aliasing forces compilers to avoid optimizing code as best as they should. We use local variables to fix this
problem.

For all examples shown below, assume `context` is a pointer to a heap allocated
struct.

## Rule 1: Scalars inside a loop

If booleans or integers gets modified inside a loop, always assign them to
local variables before doing so. Yes there are other ways to make the compiler
optimize this specific scenario, but we do this to easily **guarantee** speed.

```c
// SLOW
//
for (int i = 0; i < 100; i++) {
    // Forces a write to memory 100 times.
    //
    context->counter++;

    // This stopped context->counter from being kept in a CPU Register..
    //
    // The compiler doesnt know what do_something() does. It might write to
    // context->counter. 
    // 
    // The compiler is forced to:
    //
    // 1. Save `counter` from register to RAM.
    // 2. Calls do_something().
    // 3. Reloads `counter` fron ram to register.
    //
    do_something(context)

}
```

### Register Only Approach

```c
// FASTEST

// Load from memory to register once.
//
uint32_t count = context->counter;

for (int i = 0; i < 100; i++) {
    // Modify register directly. Zero memory access here.
    //
    count++; 

    // Pass the register value to the helper. We pass 'count' directly to avoid
    // the memory store.
    //
    do_something(context, count);
}

// Store from register back to memory once.
// 
context->counter = count;
```

### Hybrid Aprpoach (Optimized Load, Forced Store)

We do this if we cannot change the function signature.

```c
// FAST

uint32_t count = context->counter;

for (int i = 0; i < 100; i++) {
    count++; 

    // We must sync memory because do_something() reads it. This costs us 1 
    // STORE per loop, but we still save the LOAD.
    // 
    context->counter = count;

    do_something(context);
}

// No final store needed, we kept it in sync the whole time.
```

## Rule 2: Arrays inside a loop

`struct->array[i]` forces the CPU to calculate `Base + Offset + (i * 4)` every
time. `*cursor++` is just `Pointer + 4`.

```c
// SLOW
//
for (int i = 0; i < 100; ++i) {
    context->instructions[i] = ...; // Math overhead + Aliasing fear.
}

// FAST
//
bal_instruction_t *cursor = context->instructions;
for (int i = 0; i < 100; ++i) {
    *cursor++ = ...; // Simple increment.
}
```

## Rule 3: Constants/Read-Only Data

Access these values directly, like `context->max_buffer_size` or
`context->exception_level`. If we never write to it, the compiler knows it
will not change. It will load once and get cached automatically.
