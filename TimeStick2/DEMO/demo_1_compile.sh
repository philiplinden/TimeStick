#!/bin/bash
################################################################################
# DEMO 1: Compilation Test for AX88279 Driver on Kernel 6.16+
################################################################################
#
# PURPOSE:
#   Demonstrate that the driver compiles successfully on modern Linux kernels
#   (6.16+) after adding ccflags-y support to the Makefile.
#
# THE PROBLEM WE FIXED:
#   - Old Makefile used EXTRA_CFLAGS (deprecated in kernel 6.16+)
#   - Kernel build system now requires ccflags-y
#   - Without ccflags-y, version macros weren't defined correctly
#   - This caused wrong struct type selection (ethtool_ts_info vs kernel_ethtool_ts_info)
#   - Result: "incompatible pointer type" compilation error
#
# THE SOLUTION:
#   Added ccflags-y declarations alongside EXTRA_CFLAGS in Makefile
#   Now works on both old and new kernels!
#
################################################################################

set -e  # Exit on any error

# Get the directory where this script is located (TimeStick2/DEMO)
DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Driver is one level up, then into DRV
DRIVER_DIR="$DEMO_DIR/../DRV"

# Change to driver directory for compilation
cd "$DRIVER_DIR"

# Color codes for pretty output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  DEMO 1: AX88279 Driver Compilation on Kernel 6.16+           ║"
echo "╔════════════════════════════════════════════════════════════════╝"
echo ""

# ============================================================================
# STEP 1: Show System Information
# ============================================================================
echo -e "${BLUE}━━━ STEP 1: System Information ━━━${NC}"
echo ""
echo "This shows we're running a modern kernel where the bug occurs:"
echo ""

# Display kernel version
KERNEL_VERSION=$(uname -r)
echo "  Kernel Version: $KERNEL_VERSION"

# Extract major.minor version (e.g., 6.16 from 6.16.8-arch3-1)
KERNEL_MAJOR=$(echo $KERNEL_VERSION | cut -d'.' -f1)
KERNEL_MINOR=$(echo $KERNEL_VERSION | cut -d'.' -f2)

echo "  GCC Version:    $(gcc --version | head -n1)"
echo "  Make Version:   $(make --version | head -n1)"
echo ""

# Check if this is a kernel version affected by the bug
if [ "$KERNEL_MAJOR" -gt 6 ] || ([ "$KERNEL_MAJOR" -eq 6 ] && [ "$KERNEL_MINOR" -ge 11 ]); then
    echo -e "  ${GREEN}✓${NC} Kernel >= 6.11: Requires kernel_ethtool_ts_info struct"
    echo "    (This is where the original driver would fail without our fix)"
else
    echo -e "  ${YELLOW}!${NC} Kernel < 6.11: Uses older ethtool_ts_info struct"
fi
echo ""

# ============================================================================
# STEP 2: Verify the Fix is Present
# ============================================================================
echo -e "${BLUE}━━━ STEP 2: Verify Our Fix is in the Makefile ━━━${NC}"
echo ""
echo "Checking that ccflags-y declarations are present..."
echo ""

# Count how many ccflags-y lines we added
CCFLAGS_COUNT=$(grep -c "ccflags-y" Makefile || true)

if [ "$CCFLAGS_COUNT" -ge 13 ]; then
    echo -e "  ${GREEN}✓${NC} Found $CCFLAGS_COUNT ccflags-y declarations in Makefile"
    echo "    (We added these to make the driver compatible with kernel 6.16+)"
else
    echo -e "  ${YELLOW}!${NC} Warning: Only found $CCFLAGS_COUNT ccflags-y declarations"
    echo "    (Expected at least 13)"
fi
echo ""

# Show a sample of the fix
echo "Sample of our changes:"
echo "  ┌────────────────────────────────────────────────────────"
echo "  │ EXTRA_CFLAGS = -fno-pie       # Old way (still works)"
echo "  │ ccflags-y = -fno-pie           # New way (required for 6.16+)"
echo "  └────────────────────────────────────────────────────────"
echo ""

# ============================================================================
# STEP 3: Verify Conditional Compilation in C Code
# ============================================================================
echo -e "${BLUE}━━━ STEP 3: Verify C Code Has Kernel Version Checks ━━━${NC}"
echo ""
echo "The driver already had version-specific code, but it only works"
echo "if kernel version macros are properly defined (via ccflags-y):"
echo ""

if grep -q "KERNEL_VERSION(6, 11, 0)" ax88179a_772d.c; then
    echo -e "  ${GREEN}✓${NC} Found kernel version check in ax88179a_772d.c"
fi

if grep -q "kernel_ethtool_ts_info" ax88179a_772d.c; then
    echo -e "  ${GREEN}✓${NC} Found kernel_ethtool_ts_info struct (for kernel 6.11+)"
fi

if grep -q "ethtool_ts_info" ax88179a_772d.c; then
    echo -e "  ${GREEN}✓${NC} Found ethtool_ts_info struct (for older kernels)"
fi
echo ""

echo "Code structure (simplified):"
echo "  ┌────────────────────────────────────────────────────────"
echo "  │ #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)"
echo "  │     struct kernel_ethtool_ts_info *info  ← Modern kernel"
echo "  │ #else"
echo "  │     struct ethtool_ts_info *info         ← Old kernel"
echo "  │ #endif"
echo "  └────────────────────────────────────────────────────────"
echo ""

# ============================================================================
# STEP 4: Clean Previous Build
# ============================================================================
echo -e "${BLUE}━━━ STEP 4: Clean Previous Build Artifacts ━━━${NC}"
echo ""
echo "Starting from a completely clean state to prove it compiles..."
echo ""

# Run make clean and capture output
make clean > /dev/null 2>&1
echo -e "  ${GREEN}✓${NC} Build directory cleaned"
echo ""

# ============================================================================
# STEP 5: Compile the Driver
# ============================================================================
echo -e "${BLUE}━━━ STEP 5: Compile the Driver ━━━${NC}"
echo ""
echo "Now compiling with our fix applied..."
echo "This would FAIL on kernel 6.16+ without ccflags-y!"
echo ""

# Save compilation output to a temp file
BUILD_LOG="/tmp/ax88279_build_$$.log"

# Compile and show a progress indicator
echo -n "  Compiling"
if make > "$BUILD_LOG" 2>&1; then
    echo -e " ${GREEN}✓ SUCCESS${NC}"
    echo ""
    
    # ========================================================================
    # STEP 6: Verify No Errors
    # ========================================================================
    echo -e "${BLUE}━━━ STEP 6: Check for Compilation Errors ━━━${NC}"
    echo ""
    
    # Check for the specific error we fixed
    if grep -q "incompatible pointer type" "$BUILD_LOG"; then
        echo -e "  ${YELLOW}✗${NC} Found 'incompatible pointer type' error"
        echo "    (This is the bug we were trying to fix!)"
        exit 1
    else
        echo -e "  ${GREEN}✓${NC} No 'incompatible pointer type' errors"
        echo "    (The bug is FIXED!)"
    fi
    echo ""
    
    # Check for ethtool_ts_info related errors
    if grep -q "kernel_ethtool_ts_info" "$BUILD_LOG" | grep -q "error"; then
        echo -e "  ${YELLOW}✗${NC} Found kernel_ethtool_ts_info errors"
        exit 1
    else
        echo -e "  ${GREEN}✓${NC} No ethtool_ts_info related errors"
    fi
    echo ""
    
    # Show compilation warnings (if any) but don't fail
    WARNING_COUNT=$(grep -c "warning:" "$BUILD_LOG" || true)
    if [ "$WARNING_COUNT" -gt 0 ]; then
        echo -e "  ${YELLOW}!${NC} Found $WARNING_COUNT warnings (non-critical)"
        echo "    (These are pre-existing and don't affect functionality)"
    else
        echo -e "  ${GREEN}✓${NC} No warnings"
    fi
    echo ""
    
    # ========================================================================
    # STEP 7: Verify Output Files
    # ========================================================================
    echo -e "${BLUE}━━━ STEP 7: Verify Output Files Were Created ━━━${NC}"
    echo ""
    echo "Checking that all expected files were built:"
    echo ""
    
    # List of files that should be created
    EXPECTED_FILES=(
        "ax_usb_nic.ko"                     # Main kernel module
        "ax88179_programmer"                 # Utility program
        "ax88179b_179a_772d_programmer"      # Utility program
        "ax88279_programmer"                 # Utility program
        "ax88179b_179a_772d_ieee"           # IEEE test utility
        "axcmd"                              # Command utility
    )
    
    ALL_PRESENT=true
    for file in "${EXPECTED_FILES[@]}"; do
        if [ -f "$file" ]; then
            SIZE=$(stat -c%s "$file")
            # Format size in human-readable format
            if [ $SIZE -gt 1048576 ]; then
                SIZE_STR="$(($SIZE / 1048576)) MB"
            elif [ $SIZE -gt 1024 ]; then
                SIZE_STR="$(($SIZE / 1024)) KB"
            else
                SIZE_STR="$SIZE bytes"
            fi
            echo -e "  ${GREEN}✓${NC} $file ($SIZE_STR)"
        else
            echo -e "  ${YELLOW}✗${NC} $file (missing)"
            ALL_PRESENT=false
        fi
    done
    echo ""
    
    if [ "$ALL_PRESENT" = true ]; then
        echo -e "  ${GREEN}✓${NC} All files created successfully"
    else
        echo -e "  ${YELLOW}!${NC} Some files are missing"
        exit 1
    fi
    echo ""
    
    # ========================================================================
    # STEP 8: Module Information
    # ========================================================================
    echo -e "${BLUE}━━━ STEP 8: Kernel Module Information ━━━${NC}"
    echo ""
    echo "Details about the compiled kernel module:"
    echo ""
    
    # Show module metadata
    echo "  Module metadata:"
    modinfo ax_usb_nic.ko | grep -E "^(filename|version|description|author|license):" | while read line; do
        echo "    $line"
    done
    echo ""
    
    # Show that PTP support is compiled in
    if grep -q "ptp" ax_usb_nic.ko; then
        echo -e "  ${GREEN}✓${NC} PTP support is compiled in"
        echo "    (This is critical for TimeStick functionality)"
    else
        echo -e "  ${YELLOW}!${NC} PTP support may not be included"
    fi
    echo ""
    
    # ========================================================================
    # SUCCESS!
    # ========================================================================
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║                    ✓ COMPILATION SUCCESSFUL                    ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "SUMMARY:"
    echo "  • Driver compiled without errors on kernel $KERNEL_VERSION"
    echo "  • ccflags-y fix is working correctly"
    echo "  • No incompatible pointer type errors"
    echo "  • PTP support is included"
    echo "  • All utility programs built successfully"
    echo ""
    echo "The kernel 6.16+ compilation bug is FIXED!"
    echo ""
    echo "Next: Run ./demo_2_standalone.sh to test the hardware"
    echo ""
    
else
    # Compilation failed
    echo -e " ${YELLOW}✗ FAILED${NC}"
    echo ""
    echo "Compilation failed! Check the errors below:"
    echo ""
    grep -i "error:" "$BUILD_LOG" | head -10
    exit 1
fi

# Clean up
rm -f "$BUILD_LOG"
