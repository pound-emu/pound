# Pinned Submodule Versions

## Overview
This document tracks all pinned third-party submodules in the Pound project. Each submodule is locked to a specific commit hash to ensure reproducible builds and prevent unexpected updates.

## Update Policy
**NEVER update submodule commits without explicit approval from the project lead.** All updates must:
1. Be tested thoroughly
2. Have documented justification
3. Update this document with the new commit hash
4. Be committed as a separate, clear change

### ImGui
- **Repository**: https://github.com/ocornut/imgui.git
- **Version Tag**: v1.92.3
- **Pinned Commit**: bf75bfec48fc00f532af8926130b70c0e26eb099:
- **License**: MIT
- **Purpose**: Provides the graphical user interface for Pound.
- **Pinning Date**: 2025-09-20
- **Pinning Reason**: Provides the UI functionality we need with no known security issues
- **Last Review**: 2025-09-20
- **Next Review**: 2026-03-20

### SDL3
- **Repository**: https://github.com/libsdl-org/SDL
- **Version Tag**: v3.2.22
- **Commit Hash**: a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03
- **License**: Zlib
- **Purpose**: Provides the backend renderer for ImGui.
- **Pinning Date**: 2025-09-20
- **Pinning Reason**: Provides the UI render backend functionality we need with no known security issues
- **Last Review**: 2025-09-20
- **Next Review**: 2026-03-20

