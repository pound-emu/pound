# Updating Git Submodules

1. **Update to New Version (Manual)**
```bash
cd 3rd_party/imgui #imgui for example
git fetch origin
git tag -l "v*"  # List available versions
git checkout v1.90.0  # Or desired version
cd ../..
git add 3rd_party/imgui
git commit -m "Update ImGui to v1.90.0"
git submodule update --init --recursive
```
2. **Update to New Version (Automatated)**
`scripts/update_submodule.sh`

3. **Update Documentation**
   - Update relevant information in PINNED_DEPENDENCIES.md


## For Security Updates
1. Identify the security vulnerability and the specific commit that fixes it
2. Review the changes between current pinned commit and the fix
3. Test the updated version thoroughly
4. Update the submodule to the fixing commit
5. Update this document with the new commit hash
6. Commit with clear message with `scripts/update_submodule.sh`

## For Feature Updates
1. Justify why the feature update is necessary
2. Get approval from project lead
3. Review all changes between current and new version
4. Test thoroughly including regression testing
5. Update the submodule to the new commit
6. Update this document
7. Commit with clear message with `scripts/update_submodule.sh`

## For Emergency Rollbacks
If a pinned commit introduces issues:
1. Immediately revert to the previous known-good commit
2. Document the issue and reason for rollback
3. Update this document
4. Commit with clear message with `scripts/update_submodule.sh`
