#!/bin/bash
################################################################################
# Quick Verification: Demo Setup Check
################################################################################
#
# Run this before your demo to make sure everything is ready!
#
################################################################################

# Get the directory where this script is located (TimeStick2/DEMO)
DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DEMO_DIR"

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Pre-Demo Verification                                         ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

ALL_GOOD=true

# Check demo scripts exist
echo "Checking demo scripts..."
for script in demo_1_compile.sh demo_2_standalone.sh demo_3_ptp_sync.sh; do
    if [ -x "$script" ]; then
        echo -e "  ${GREEN}✓${NC} $script (executable)"
    elif [ -f "$script" ]; then
        echo -e "  ${YELLOW}!${NC} $script (not executable - fixing...)"
        chmod +x "$script"
    else
        echo -e "  ${RED}✗${NC} $script (missing!)"
        ALL_GOOD=false
    fi
done
echo ""

# Check documentation
echo "Checking documentation..."
for doc in DEMO_README.md QUICK_START.md DEMO_CHECKLIST.md; do
    if [ -f "$doc" ]; then
        echo -e "  ${GREEN}✓${NC} $doc"
    else
        echo -e "  ${RED}✗${NC} $doc (missing!)"
        ALL_GOOD=false
    fi
done
echo ""

# Check driver directory
echo "Checking driver directory..."
if [ -d "../DRV" ]; then
    echo -e "  ${GREEN}✓${NC} ../DRV/ exists"
    
    if [ -f "../DRV/Makefile" ]; then
        echo -e "  ${GREEN}✓${NC} Makefile found"
        
        # Check for our fix
        CCFLAGS_COUNT=$(grep -c "ccflags-y" ../DRV/Makefile 2>/dev/null || echo "0")
        if [ "$CCFLAGS_COUNT" -ge 13 ]; then
            echo -e "  ${GREEN}✓${NC} ccflags-y fix present ($CCFLAGS_COUNT lines)"
        else
            echo -e "  ${YELLOW}!${NC} Only $CCFLAGS_COUNT ccflags-y lines (expected 15)"
        fi
    else
        echo -e "  ${RED}✗${NC} Makefile not found"
        ALL_GOOD=false
    fi
    
    if [ -f "../DRV/ax88179a_772d.c" ]; then
        echo -e "  ${GREEN}✓${NC} Driver source found"
    else
        echo -e "  ${RED}✗${NC} Driver source not found"
        ALL_GOOD=false
    fi
else
    echo -e "  ${RED}✗${NC} ../DRV/ not found"
    ALL_GOOD=false
fi
echo ""

# Check system
echo "Checking system requirements..."
echo -e "  Kernel: $(uname -r)"

if command -v gcc > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} GCC installed"
else
    echo -e "  ${RED}✗${NC} GCC not installed"
    ALL_GOOD=false
fi

if command -v make > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} Make installed"
else
    echo -e "  ${RED}✗${NC} Make not installed"
    ALL_GOOD=false
fi

if [ -d "/lib/modules/$(uname -r)/build" ]; then
    echo -e "  ${GREEN}✓${NC} Kernel headers installed"
else
    echo -e "  ${YELLOW}!${NC} Kernel headers may not be installed"
fi
echo ""

# Check hardware (optional)
echo "Checking hardware (optional)..."
if lsusb 2>/dev/null | grep -q "0b95:1790\|AX88279\|ASIX.*Gigabit"; then
    echo -e "  ${GREEN}✓${NC} TimeStick detected (USB connected)"
else
    echo -e "  ${YELLOW}!${NC} TimeStick not detected (plug it in before Demo 2)"
fi
echo ""

# Check optional tools
echo "Checking optional tools (for Demo 3)..."
if command -v ethtool > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} ethtool installed"
else
    echo -e "  ${YELLOW}!${NC} ethtool not installed (needed for hardware test)"
fi

if command -v ptp4l > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} linuxptp installed (ptp4l available)"
else
    echo -e "  ${YELLOW}!${NC} linuxptp not installed (only needed for Demo 3)"
    echo "    Install with: sudo pacman -S linuxptp"
fi
echo ""

# Final verdict
echo "═══════════════════════════════════════════════════════════════"
if [ "$ALL_GOOD" = true ]; then
    echo -e "${GREEN}✓ All checks passed! You're ready for the demo.${NC}"
    echo ""
    echo "Quick start:"
    echo "  ./demo_1_compile.sh"
    echo "  sudo ./demo_2_standalone.sh"
    echo ""
    echo "For reference, open: QUICK_START.md"
else
    echo -e "${RED}✗ Some checks failed. Fix the issues above before the demo.${NC}"
fi
echo "═══════════════════════════════════════════════════════════════"
echo ""
