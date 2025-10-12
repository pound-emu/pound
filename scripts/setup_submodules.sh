#!/bin/bash
# Setup submodules with proper pinning configuration
# DISCLAIMER: This was written by AI. GLM-4.5 wrote this better than I ever could.

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/log.sh"

set -uo pipefail errtrace
IFS=$'\n\t'

# Global variables
SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git rev-parse --show-toplevel)"

# Status tracking
declare -i VALIDATION_ERRORS=0
declare -i CONFIGURATION_ERRORS=0
declare -i INITIALIZATION_ERRORS=0

# =============================================================================
# VALIDATION FUNCTIONS
# =============================================================================

validate_environment() {
    log_info "Validating environment..."

    # Check if we're in the correct repository
    if [[ ! -f ".gitmodules" ]]; then
        log_critical "Not in pound repository root or .gitmodules missing"
        exit 1
    fi

    # Check required tools
    local required_tools=("git" "bash" "mkdir" "rm" "tee")
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            log_critical "Required tool '$tool' not found in PATH"
            exit 1
        fi
    done

    # Check git repository state
    if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        log_critical "Not inside a git repository"
        exit 1
    fi

    # Check for uncommitted changes
    if ! git diff-index --quiet HEAD --; then
        log_warn "Uncommitted changes detected - this may affect submodule setup"
    fi

    # Check disk space
    local available_space
    available_space=$(df . | awk 'NR==2 {print $4}')
    if [[ $available_space -lt 1048576 ]]; then  # Less than 1GB
        log_warn "Low disk space available: $available_space KB"
    fi

    log_info "Environment validation complete."
}

validate_submodules() {
    log_info "Validating submodule configuration..."

    # Check if .gitmodules exists and is readable
    if [[ ! -r ".gitmodules" ]]; then
        log_critical ".gitmodules file not found or not readable"
        exit 1
    fi

    # Parse and validate submodule configurations
    local submodules=()
    while IFS= read -r line; do
        if [[ $line =~ ^\[submodule\ \"(.*)\"\]$ ]]; then
            submodules+=("${BASH_REMATCH[1]}")
        fi
    done < .gitmodules

    if [[ ${#submodules[@]} -eq 0 ]]; then
        log_critical "No submodules found in .gitmodules"
        exit 1
    fi

    log_info "Found ${#submodules[@]} submodules: ${submodules[*]}"

    # Validate each submodule configuration
    for submodule in "${submodules[@]}"; do
        local submodule_path
        local submodule_url

        submodule_path=$(git config -f .gitmodules --get submodule."$submodule".path)
        submodule_url=$(git config -f .gitmodules --get submodule."$submodule".url)

        if [[ -z "$submodule_path" ]]; then
            log_error "Missing path configuration for submodule '$submodule'"
            ((VALIDATION_ERRORS++))
        fi

        if [[ -z "$submodule_url" ]]; then
            log_error "Missing URL configuration for submodule '$submodule'"
            ((VALIDATION_ERRORS++))
        fi

        # Validate URL format
        if [[ "$submodule_url" =~ ^(https?|git):// ]] || [[ "$submodule_url" =~ ^git@ ]]; then
            log_info "Submodule '$submodule' URL format valid: $submodule_url"
        else
            log_error "Invalid URL format for submodule '$submodule': $submodule_url"
            ((VALIDATION_ERRORS++))
        fi
    done

    if [[ $VALIDATION_ERRORS -gt 0 ]]; then
        log_critical "Submodule configuration validation failed with $VALIDATION_ERRORS errors"
        exit 1
    fi

    log_info "Submodule configuration validation complete."
}

# =============================================================================
# EXECUTION FUNCTIONS
# =============================================================================

initialize_submodules() {
    local max_retries=3
    local retry_delay=5
    local success_count=0
    local failure_count=0

    # Get list of submodules
    local submodules=()
    local submodule_paths=()

    # Parse .gitmodules file
    while IFS= read -r line; do
        if [[ $line =~ ^\[submodule\ \"(.*)\"\]$ ]]; then
            local submodule_name="${BASH_REMATCH[1]}"
            submodules+=("$submodule_name")

            # Get the path for this submodule
            local submodule_path
            submodule_path=$(git config -f .gitmodules --get submodule."$submodule_name".path)
            submodule_paths+=("$submodule_path")
        fi
    done < .gitmodules

    log_info "Found ${#submodules[@]} submodules to initialize"

    # Process each submodule
    for i in "${!submodules[@]}"; do
        local submodule="${submodules[i]}"
        local submodule_path="${submodule_paths[i]}"

        log_info "Submodule path: '$submodule_path'"

        # Try to initialize the submodule
        local submodule_initialized=false

        for ((attempt=1; attempt<=max_retries; attempt++)); do
            log_info "Attempt $attempt of $max_retries for submodule '$submodule'"

            # Execute git command with explicit error handling
            local output
            output=$(git submodule update --init "$submodule_path" 2>&1)
            local exit_code=$?

            if [[ $exit_code -eq 0 ]]; then
                log_info "Successfully initialized submodule '$submodule'"
                ((success_count++))
                submodule_initialized=true
                break
            else
                log_warn "Failed to initialize submodule '$submodule' (exit code: $exit_code)"
                log_warn "Git output: $output"

                if [[ $attempt -lt $max_retries ]]; then
                    log_info "Waiting $retry_delay seconds before retry..."
                    sleep $retry_delay
                else
                    log_error "Failed to initialize submodule '$submodule' after $max_retries attempts"
                    ((failure_count++))
                fi
            fi
        done
    done
    log_info "Results: $success_count successful, $failure_count failed"

    # Return success even if some submodules failed
    # We'll handle failures in the validation phase
    return 0
}

configure_submodules() {
    log_info "Configuring submodules to prevent automatic updates..."

    # Get all configured submodules
    local submodules=()
    while IFS= read -r line; do
        if [[ $line =~ ^\[submodule\ \"(.*)\"\]$ ]]; then
            submodules+=("${BASH_REMATCH[1]}")
        fi
    done < .gitmodules

    local configured_count=0
    local failed_count=0

    for submodule in "${submodules[@]}"; do
        log_info "Configuring submodule: $submodule"

        if git config "submodule.$submodule.update" none; then
            log_info "Successfully configured $submodule"
            ((configured_count++))
        else
            log_error "Failed to configure $submodule"
            ((failed_count++))
        fi
    done

    log_info "Configuration complete: $configured_count successful, $failed_count failed"

    # Return success even if some configurations failed
    return 0
}

validate_setup() {
    log_info "Validating submodule setup..."

    local validation_errors=0

    # Check each submodule
    while IFS= read -r line; do
        if [[ $line =~ ^\[submodule\ \"(.*)\"\]$ ]]; then
            local submodule="${BASH_REMATCH[1]}"
            local submodule_path
            submodule_path=$(git config -f .gitmodules --get submodule."$submodule".path)

            # Check if submodule directory exists
            if [[ ! -d "$submodule_path" ]]; then
                log_error "Submodule directory '$submodule_path' does not exist"
                ((validation_errors++))
                continue
            fi

            # Check if it's a valid git repository
            if [[ ! -f "$submodule_path/.git" ]]; then
                log_error "Submodule '$submodule_path' is not a valid git repository"
                ((validation_errors++))
                continue
            fi

            # Check if submodule is properly initialized
            if ! (cd "$submodule_path" && git rev-parse --git-dir >/dev/null 2>&1); then
                log_error "Submodule '$submodule_path' is not properly initialized"
                ((validation_errors++))
                continue
            fi

            # Check update configuration
            local update_config
            update_config=$(git config submodule."$submodule".update)
            if [[ "$update_config" != "none" ]]; then
                log_error "Submodule '$submodule' update configuration is '$update_config', expected 'none'"
                ((validation_errors++))
            fi

            log_info "Submodule '$submodule' validation passed"
        fi
    done < .gitmodules

    if [[ $validation_errors -gt 0 ]]; then
        log_critical "Setup validation failed with $validation_errors errors"
        exit 1
    fi

    log_info "All submodules validated successfully"
    return 0
}

# =============================================================================
# CLEANUP FUNCTIONS
# =============================================================================

cleanup_on_failure() {
    log_warn "Initiating cleanup due to failure..."

    # Remove any partially initialized submodules
    while IFS= read -r line; do
        if [[ $line =~ ^\[submodule\ \"(.*)\"\]$ ]]; then
            local submodule="${BASH_REMATCH[1]}"
            local submodule_path
            submodule_path=$(git config -f .gitmodules --get submodule."$submodule".path)

            if [[ -d "$submodule_path" ]]; then
                log_info "Cleaning up partially initialized submodule: $submodule_path"
                rm -rf "$submodule_path"
            fi
        fi
    done < .gitmodules

    # Clean up temporary files
    if [[ -d "$TEMP_DIR" ]]; then
        log_info "Cleaning up temporary directory: $TEMP_DIR"
        rm -rf "$TEMP_DIR"
    fi

    log_info "Cleanup complete"
}

# =============================================================================
# DISPLAY FUNCTIONS
# =============================================================================

display_header() {
    cat << 'EOF'
===============================================================================
                            POUND SUBMODULE SETUP
===============================================================================

This script configures submodules with commit pinning for stability.

WARNING: This operation modifies Git configuration and affects repository state.

Result:
- Submodules will be initialized and pinned to specific commits
- Automatic updates will be disabled

===============================================================================
EOF
}

display_summary() {
    cat << 'EOF'
===============================================================================
                                    SUMMARY
===============================================================================

Submodule setup completed successfully.

SYSTEM STATUS: STABLE
SUBMODULES: PINNED
AUTOMATIC UPDATES: DISABLED

Next Steps:
1. Verify submodule configurations in .gitmodules
2. Review pinned versions in 3rd_party/PINNED_VERSIONS.md
3. Test build system with pinned submodules

===============================================================================
EOF
}

# =============================================================================
# MAIN
# =============================================================================

main() {
    # Display mission header
    display_header

    # Change to repository root
    cd "$REPO_ROOT"

    # Execute phases
    validate_environment
    validate_submodules
    initialize_submodules
    configure_submodules
    validate_setup

    display_summary

    log_info "Submodule setup successful"
    exit 0
}

# Execute main function
main "$@"
