**Design Document: Third-Party Code Inclusion Strategy**

**Author:** GloriousTacoo, Lead Developer   
**Status:** FINAL  
**Version:** 1.0  
**Date:** 2025-09-20  

*Disclaimer: This document was mostly written by AI. I'm not a good technical writer.*

### **1. Problem Statement**

We require a rigorous, auditable, and maintainable strategy for including third-party code in the Pound Virtual Machine. The current approach lacks formal standardization, creating risks to system integrity, security, and long-term maintainability. Each third-party inclusion represents a potential attack surface, a source of unpredictable behavior, and a maintenance burden that must be managed with extreme prejudice. We cannot afford the luxury of casual dependency management in a system that demands absolute reliability. The current ad-hoc approach must be replaced with a formal methodology that prioritizes safety, auditability, and control above all other considerations.

### **2. Glossary**

Third-party code refers to any software component not developed in-house as part of the Pound project, including libraries, frameworks, and tools that are incorporated into our build system. Vendoring is the practice of copying third-party source code directly into the project repository, creating a local snapshot that is version-controlled alongside our own code. Submodules are Git mechanisms that link to external repositories while maintaining version references within our main repository. CMake FetchContent is a modern CMake feature that downloads and configures dependencies at build time, integrating them directly into the build process without requiring separate management. Dependency pinning refers to the practice of locking to specific versions of third-party code to prevent unexpected updates from introducing instability or security vulnerabilities.

### **3. Breaking Changes**

Any transition to a new third-party inclusion strategy will require immediate and comprehensive refactoring of the existing build system. All current third-party dependencies must be evaluated against the new standard, with non-compliant components either brought into compliance or removed from the project. The CMakeLists.txt files throughout the project hierarchy will require complete rewriting to implement the chosen strategy. Continuous integration pipelines must be updated to enforce compliance. This is not a gradual transition but a complete overhaul of our dependency management philosophy that will affect every aspect of the build process.

### **4. Success Criteria**

A successful third-party inclusion strategy must provide complete auditability of all dependencies, with clear documentation of the origin, version, and purpose of each external component. The build process must be fully reproducible across all supported platforms, ensuring that any build from the same source produces identical binaries. Security vulnerabilities in third-party components must be detectable through automated scanning, with clear processes for updating affected dependencies. The strategy must minimize the risk of dependency conflicts and version incompatibilities while maintaining build performance. All third-party code must be subject to the same rigorous coding standards and assertion framework as our own code, with no exceptions granted for external components.

### **5. Proposed Design**

Our philosophy must be that third-party code is not trusted by default but rather treated as potentially hostile until proven otherwise through rigorous evaluation and continuous monitoring. The strategy must prioritize control over convenience, ensuring that every line of third-party code included in the project is explicitly approved, documented, and monitored throughout its lifecycle. We will implement a defense-in-depth approach where each dependency is evaluated not just for functionality but for security posture, maintenance status, and compatibility with our fail-fast philosophy. The build system must enforce strict version control and prevent automatic updates that could introduce unexpected behavior or security vulnerabilities.

### **6. Technical Design**

The recommended approach is direct vendoring of all third-party code into a dedicated 3rd_Party directory with strict version control and comprehensive documentation. Each vendored dependency must reside in its own subdirectory with clear attribution, license information, and a README detailing its purpose and any modifications made. The build system will reference these local copies exclusively, eliminating any external network dependencies during the build process. All vendored code must be subjected to the same static analysis and coding standards as our own code, with automated checks ensuring compliance. A manifest file will track all dependencies, their versions, origins, and security status, with automated tools regularly checking for known vulnerabilities in the included components.

### **7. Components**

The strategy involves several key components working together to ensure safe dependency management. The 3rd_Party directory serves as the centralized repository for all external code, with each dependency isolated in its own version-controlled subdirectory. The build system, primarily CMake, will be configured to build exclusively from these local copies, with no external network access during compilation. A dependency manifest will maintain metadata about each included component, including version hashes, license information, and security status. Automated tooling will continuously monitor these dependencies for security vulnerabilities and compliance with our coding standards. Documentation requirements mandate that each dependency include clear attribution, modification logs, and integration notes.

### **8. Dependencies**

This strategy depends on several critical elements to be effective. The version control system must provide robust support for large binary files and complete history tracking to maintain the integrity of vendored components. The build system must be capable of handling complex dependency graphs without requiring network access during compilation. Automated security scanning tools must be integrated into the development workflow to continuously monitor for vulnerabilities in third-party components. Developer training and enforcement mechanisms are essential to ensure compliance with the new standards. Legal review processes must be in place to verify license compatibility and ensure that all third-party inclusions meet our open-source requirements.

### **9. Major Risks & Mitigations**

The primary risk is that vendoring large third-party components will bloat the repository size, potentially impacting clone times and storage requirements. This can be mitigated through Git LFS (Large File Storage) and careful selection of only essential components. Another significant risk is the potential for falling behind on security updates if the vendored approach makes updates more cumbersome. This will be addressed through automated monitoring tools and streamlined update processes that minimize the effort required to apply security patches.

### **10. Out of Scope**

This strategy does not address the evaluation of specific third-party libraries for technical suitability, as that is covered by separate architectural review processes. The approach does not provide guidance on negotiating licenses or legal agreements with third-party vendors, as those matters fall under legal review. The strategy does not cover the integration of proprietary or commercially licensed components, as Pound is an open-source project with specific licensing requirements. Continuous integration testing of third-party components is considered part of the broader testing strategy and not specifically addressed here. The strategy also does not address the long-term maintenance of deprecated third-party dependencies, as those should be removed rather than maintained.

### **11. Alternatives Considered**

The primary alternative to direct vendoring is the use of Git submodules, which maintain references to external repositories while keeping them logically separate from the main codebase. While this approach reduces repository bloat, it introduces significant complexity in dependency management and makes builds less reliable due to external network dependencies. Another alternative is CMake FetchContent, which downloads dependencies at build time. This approach offers convenience but sacrifices reproducibility and control, as builds become dependent on external network availability and the continued existence of remote repositories. Package managers like vcpkg or Conan were also considered but rejected due to their complexity and the additional attack surface they introduce. Each of these alternatives was ultimately deemed unsuitable for a safety-critical system where control and reproducibility are paramount.

### **12. Recommendation**

After careful analysis of all alternatives, direct vendoring with strict version control represents the superior approach for the Pound project. While it requires more discipline and introduces repository size considerations, it provides the level of control, auditability, and reproducibility demanded by a safety-critical system. The current approach of mixed dependency management methods must be replaced entirely with this standardized vendoring strategy. The benefits of complete control, offline builds, and comprehensive auditability far outweigh the inconvenience of managing larger repositories. This approach aligns perfectly with our fail-fast philosophy and ensures that every line of code in the project, whether first-party or third-party, is subject to the same rigorous standards of safety and reliability. The transition to this approach should begin immediately with a complete inventory of all current third-party dependencies.
