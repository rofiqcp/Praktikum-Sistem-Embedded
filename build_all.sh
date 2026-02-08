#!/bin/bash
# =============================================================================
# build_all.sh — Build Verification Script
# Praktikum Sistem Embedded
# 
# Menjalankan kompilasi semua 342 project (171 ESP32 + 171 STM32)
# dan menghasilkan laporan build log.
#
# Penggunaan:
#   chmod +x build_all.sh
#   ./build_all.sh              # Build semua project
#   ./build_all.sh esp32        # Build hanya ESP32
#   ./build_all.sh stm32        # Build hanya STM32
#   ./build_all.sh Modul-01     # Build hanya Modul tertentu
#
# Output:
#   build_log_YYYYMMDD_HHMMSS.txt   — Log detail
#   build_summary_YYYYMMDD_HHMMSS.txt — Ringkasan
# =============================================================================

set -o pipefail

# Warna terminal
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Direktori dasar
BASEDIR="$(cd "$(dirname "$0")" && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
LOG_FILE="${BASEDIR}/build_log_${TIMESTAMP}.txt"
SUMMARY_FILE="${BASEDIR}/build_summary_${TIMESTAMP}.txt"

# Counters
TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0
FAILED_LIST=""

# Filter
FILTER_PLATFORM="${1:-all}"
FILTER_MODULE="${2:-all}"

# =============================================================================
# Functions
# =============================================================================

log() {
    echo "$1" | tee -a "$LOG_FILE"
}

log_header() {
    echo "" | tee -a "$LOG_FILE"
    echo "================================================================" | tee -a "$LOG_FILE"
    echo " $1" | tee -a "$LOG_FILE"
    echo "================================================================" | tee -a "$LOG_FILE"
}

build_project() {
    local project_dir="$1"
    local project_name
    project_name="$(basename "$project_dir")"
    local module_name
    module_name="$(basename "$(dirname "$(dirname "$project_dir")")")"
    local platform
    platform="$(basename "$(dirname "$project_dir")")"
    
    # Label for display
    local label="${module_name}/${platform}/${project_name}"
    
    TOTAL=$((TOTAL + 1))
    
    # Check if platformio.ini exists
    if [ ! -f "${project_dir}/platformio.ini" ]; then
        log "  [SKIP] ${label} — No platformio.ini"
        SKIPPED=$((SKIPPED + 1))
        return
    fi
    
    # Check if src/main.c exists
    if [ ! -f "${project_dir}/src/main.c" ]; then
        log "  [SKIP] ${label} — No src/main.c"
        SKIPPED=$((SKIPPED + 1))
        return
    fi
    
    # Run PlatformIO build (first environment only for speed)
    local first_env
    first_env=$(grep '^\[env:' "${project_dir}/platformio.ini" | head -1 | sed 's/\[env:\(.*\)\]/\1/')
    
    if [ -z "$first_env" ]; then
        log "  [SKIP] ${label} — No environment found"
        SKIPPED=$((SKIPPED + 1))
        return
    fi
    
    printf "  Building %-60s " "${label}"
    
    # Build with timeout (120s) and capture output
    local build_output
    build_output=$(cd "$project_dir" && timeout 120 pio run -e "$first_env" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC}" 
        log "  [PASS] ${label} (env: ${first_env})"
        PASSED=$((PASSED + 1))
    elif [ $exit_code -eq 124 ]; then
        echo -e "${YELLOW}[TIMEOUT]${NC}"
        log "  [TIMEOUT] ${label} (env: ${first_env}) — Build exceeded 120s"
        FAILED=$((FAILED + 1))
        FAILED_LIST="${FAILED_LIST}\n  TIMEOUT: ${label}"
    else
        echo -e "${RED}[FAIL]${NC}"
        log "  [FAIL] ${label} (env: ${first_env})"
        # Log last 20 lines of error output
        echo "$build_output" | tail -20 >> "$LOG_FILE"
        echo "---" >> "$LOG_FILE"
        FAILED=$((FAILED + 1))
        FAILED_LIST="${FAILED_LIST}\n  FAIL: ${label}"
    fi
}

# =============================================================================
# Main
# =============================================================================

echo -e "${BOLD}${CYAN}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║        Praktikum Sistem Embedded — Build Verifier           ║"
echo "║                                                             ║"
echo "║  ESP32 (ESP-IDF) + STM32 (STM32Cube HAL)                  ║"
echo "║  342 Projects across 14 Modules                             ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

log "Build Verification Started: $(date)"
log "Filter: platform=${FILTER_PLATFORM}, module=${FILTER_MODULE}"
log "Base directory: ${BASEDIR}"
log ""

# Check PlatformIO
if ! command -v pio &> /dev/null; then
    echo -e "${RED}ERROR: PlatformIO CLI (pio) not found!${NC}"
    echo "Install with: pip install platformio"
    exit 1
fi

log "PlatformIO version: $(pio --version)"
log ""

# Find all project directories
for modul_dir in "${BASEDIR}"/Modul-*/; do
    modul_name="$(basename "$modul_dir")"
    
    # Apply module filter
    if [ "$FILTER_MODULE" != "all" ] && [[ "$modul_name" != *"$FILTER_MODULE"* ]]; then
        continue
    fi
    
    # ESP32 projects
    if [ "$FILTER_PLATFORM" = "all" ] || [ "$FILTER_PLATFORM" = "esp32" ] || [[ "$FILTER_PLATFORM" == "Modul"* ]]; then
        esp32_dir="${modul_dir}praktikum/ESP32"
        if [ -d "$esp32_dir" ]; then
            log_header "${modul_name} / ESP32"
            for project_dir in "${esp32_dir}"/*/; do
                if [ -d "$project_dir" ]; then
                    build_project "$project_dir"
                fi
            done
        fi
    fi
    
    # STM32 projects
    if [ "$FILTER_PLATFORM" = "all" ] || [ "$FILTER_PLATFORM" = "stm32" ] || [[ "$FILTER_PLATFORM" == "Modul"* ]]; then
        stm32_dir="${modul_dir}praktikum/STM32"
        if [ -d "$stm32_dir" ]; then
            log_header "${modul_name} / STM32"
            for project_dir in "${stm32_dir}"/*/; do
                if [ -d "$project_dir" ]; then
                    build_project "$project_dir"
                fi
            done
        fi
    fi
done

# =============================================================================
# Summary
# =============================================================================

echo ""
echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}                    BUILD SUMMARY${NC}"
echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}"

SUMMARY="
Build Verification Complete: $(date)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Total projects:   ${TOTAL}
  ✅ Passed:         ${PASSED}
  ❌ Failed:         ${FAILED}
  ⏭️  Skipped:       ${SKIPPED}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Success rate:     $(( TOTAL > 0 ? (PASSED * 100) / TOTAL : 0 ))%
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo "$SUMMARY" | tee -a "$LOG_FILE"
echo "$SUMMARY" > "$SUMMARY_FILE"

if [ $FAILED -gt 0 ]; then
    echo -e "\n${RED}Failed projects:${NC}" | tee -a "$SUMMARY_FILE"
    echo -e "$FAILED_LIST" | tee -a "$LOG_FILE" | tee -a "$SUMMARY_FILE"
fi

echo ""
echo -e "📄 Detail log:   ${LOG_FILE}"
echo -e "📊 Summary:      ${SUMMARY_FILE}"

# Clean up temp files
rm -f "${BASEDIR}"/_fix_all_projects.py "${BASEDIR}"/_fix_headings.py 2>/dev/null

# Exit code based on failures
if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
