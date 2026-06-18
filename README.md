<h1 align="center">
  <img src="/resources/pound.png" height="150px">
  <br><br>
  <img src="https://img.shields.io/github/stars/pound-emu/pound" width="100">
  <a href="https://github.com/pound-emu/pound/actions?query=branch%3Amain">
    <img src="https://img.shields.io/badge/Latest Builds-Here-aa00aa.svg" width="150">
  </a>
  <br><br>
  Pound
</h1>

<p align="center"><em>“i think of getting pounded when i see that [name]” – Satisfied Customer</em></p>

## Overview

**IMPORTANT: IN ORDER TO SQUEEZE AS MUCH PERFORMANCE FOR SWITCH 1 AND 2 EMULATORS, DEVELOPMENT HAS FULLY SHIFTED TO
CREATING A [NEW ARM RECOMPILER](https://github.com/pound-emu/ballistic) FROM THE GROUND UP. IF YOU ARE A COMPILER
DEVELOPER PLEASE GIVE US YOUR SUPPORT**

Join the [**Pound Discord Server**](https://discord.gg/aMmTmKsVC7)!

- [ ] Translate SM86 to SPIR-V to Vulkan.
- [ ] Add `mimalloc` for host allocator.
- [ ] Create a custom pool / slab allocator for Horizon OS.
- [ ] Create a JIT code cache memory allocator for Ballistic.
- [ ] Create a JIT metadata manager.
- [ ] Integrate Ballistic into Pound.

If you're capable of reverse engineering the switch 2 kernel dumps when it get hacked, it would help me if you focus on figuring out the switch 2's execution and memory model (like W^X) so I can support running switch 2 code on my JIT compiler as fast as possible.
