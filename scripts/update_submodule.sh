#!/bin/bash
# Controlled update of a specific submodule

set -e

if [ $# -lt 3 ]; then
    echo "Usage: $0 <submodule_path> <new_commit_hash> <reason>"
    echo "Example: $0 3rd_party/imgui a1b2c3d4e5f6789012345678901234567890123456 \"Security fix for CVE-2025-1234\""
    exit 1
fi

SUBMODULE_PATH="$1"
NEW_COMMIT="$2"
REASON="$3"

# Get the repository root directory
REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT"

# Normalize the submodule path (remove leading ../ and trailing /)
SUBMODULE_PATH=$(echo "$SUBMODULE_PATH" | sed 's:^\.\.\/::' | sed 's:/$::')

echo "Repository root: $REPO_ROOT"
echo "Updating submodule: $SUBMODULE_PATH"
echo "New commit: $NEW_COMMIT"
echo "Reason: $REASON"
echo ""

# Check if submodule exists
if [ ! -d "$SUBMODULE_PATH" ]; then
    echo "Error: Submodule directory $SUBMODULE_PATH does not exist"
    echo "Available submodules:"
    git submodule status | awk '{print $2}'
    exit 1
fi

# Check if it's actually a submodule
if [ ! -f "$SUBMODULE_PATH/.git" ]; then
    echo "Error: $SUBMODULE_PATH is not a git submodule"
    exit 1
fi

# Show current commit
echo "Current commit:"
cd "$SUBMODULE_PATH"
CURRENT_COMMIT=$(git rev-parse HEAD)
echo "  $CURRENT_COMMIT"
git log --oneline -1
echo ""

cd "$REPO_ROOT"

# Ask for confirmation
read -p "Are you sure you want to update $SUBMODULE_PATH from $CURRENT_COMMIT to $NEW_COMMIT? (y/N): " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Update cancelled."
    exit 0
fi

# Update the submodule
echo "Updating submodule..."
cd "$SUBMODULE_PATH"
git fetch origin
if ! git checkout "$NEW_COMMIT"; then
    echo "Error: Failed to checkout commit $NEW_COMMIT"
    echo "Available commits:"
    git log --oneline -10
    cd "$REPO_ROOT"
    exit 1
fi

cd "$REPO_ROOT"

# Commit the change
echo "Committing the update..."
git add "$SUBMODULE_PATH"
git commit -m "Update $SUBMODULE_PATH to $NEW_COMMIT: $REASON"

echo "Update complete!"
echo ""
echo "IMPORTANT: Don't forget to:"
echo "1. Update 3rd_party/PINNED_VERSIONS.md with the new commit hash"
echo "2. Test the changes thoroughly"
echo "3. Update the CMakeLists.txt verification function if needed"
