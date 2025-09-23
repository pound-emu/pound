#!bin/bash

# Error handling function
error_handler() {
    local exit_code=$?
    local line_number=$1
    log_critical "Script exited with code $exit_code at line $line_number"
    log_critical "ABORT: Submodule setup failed - SYSTEM IN UNSTABLE STATE"
    cleanup_on_failure
    exit $exit_code
}

trap 'error_handler $LINENO' ERR

# Logging system
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message"
}

log_info() { log "INFO" "$*"; }
log_warn() { log "WARN" "$*"; }
log_error() { log "ERROR" "$*"; }
log_critical() { log "CRITICAL" "$*"; }

