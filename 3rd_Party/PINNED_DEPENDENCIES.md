# Pinned Submodule Versions

## Overview
This document tracks all pinned third-party submodules in the Pound project. Each submodule is locked to a specific commit hash to ensure reproducible builds and prevent unexpected updates.

## Update Policy
**NEVER update submodule commits without explicit approval from the project lead.** All updates must:
1. Be tested thoroughly
2. Have documented justification
3. Update this document with the new commit hash
4. Be committed as a separate, clear change

### GoogleTest
- **Repository**: https://github.com/google/googletest
- **Version Tag**: v1.17.0
- **Commit Hash**: 52eb8108c5bdec04579160ae17225d66034bd723
- **License**: BSD-3-Clause
- **Purpose**: Provides the testing and mocking framework for Pound.
- **Pinning Date**: 2025-11-09
- **Pinning Reason**: Dependency added for the first time.
- **Last Review**: 2025-11-09
- **Next Review**: 2026-05-09
