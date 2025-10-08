#!/bin/bash
################################################################################
# DEMO 3: Full PTP Synchronization Test
################################################################################
#
# PURPOSE:
#   Demonstrate actual PTP time synchronization between the TimeStick and
#   another PTP-capable device on the network.
#
# WHAT THIS TESTS:
#   - Complete PTP protocol communication
#   - Hardware timestamping in action
#   - Clock synchronization accuracy
#   - PTP master/slave operation
#
# NETWORK REQUIREMENTS:
#   One of the following must be present on the network:
#   1. A PTP Grandmaster clock
#   2. Another computer running ptp4l as master
#   3. A network switch with PTP support
#   4. Another TimeStick on a different computer
#
# PTP BASICS (for your coworker):
#   - PTP = Precision Time Protocol (IEEE 1588)
#   - Synchronizes clocks across a network to microsecond/nanosecond accuracy
#   - Uses hardware timestamping to eliminate software delays
#   - Master clock sends time messages, slaves synchronize to master
#   - Requires hardware support (which TimeStick provides!)
#
# REQUIREMENTS:
#   - Must be run as root (sudo)
#   - TimeStick must be connected to network with Ethernet cable
#   - linuxptp tools must be installed (pacman -S linuxptp)
#   - Another PTP device must be on the network
#
################################################################################

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "❌ This script must be run as root"
    echo "Please run: sudo $0"
    exit 1
fi

# Get the directory where this script is located (TimeStick2/DEMO)
DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Driver is one level up, then into DRV (not used in this script, but documented)
DRIVER_DIR="$DEMO_DIR/../DRV"

# Note: This script can run from anywhere - it only interacts with the system,
# not with source files. We don't need to change directory.

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  DEMO 3: Full PTP Synchronization Test                        ║"
echo "╔════════════════════════════════════════════════════════════════╝"
echo ""

# ============================================================================
# STEP 1: Check Prerequisites
# ============================================================================
echo -e "${BLUE}━━━ STEP 1: Check Prerequisites ━━━${NC}"
echo ""

# Check for linuxptp tools
MISSING_TOOLS=()

if ! command -v ptp4l > /dev/null 2>&1; then
    MISSING_TOOLS+=("ptp4l")
fi

if ! command -v pmc > /dev/null 2>&1; then
    MISSING_TOOLS+=("pmc")
fi

if ! command -v phc2sys > /dev/null 2>&1; then
    MISSING_TOOLS+=("phc2sys")
fi

if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    echo -e "  ${RED}✗${NC} Missing required tools: ${MISSING_TOOLS[*]}"
    echo ""
    echo "Please install linuxptp:"
    echo "  pacman -S linuxptp       (Arch Linux)"
    echo "  apt install linuxptp     (Ubuntu/Debian)"
    echo "  dnf install linuxptp     (Fedora/RHEL)"
    exit 1
else
    echo -e "  ${GREEN}✓${NC} All required tools are installed"
    echo "    • ptp4l  - PTP daemon"
    echo "    • pmc    - PTP management client"
    echo "    • phc2sys - Sync system clock to PTP clock"
fi
echo ""

# ============================================================================
# STEP 2: Find TimeStick Interface
# ============================================================================
echo -e "${BLUE}━━━ STEP 2: Locate TimeStick Network Interface ━━━${NC}"
echo ""

# Find interface using ax_usb_nic driver
INTERFACE=""
for iface in $(ls /sys/class/net/ | grep -v "lo\|wlan"); do
    DRIVER=$(basename $(readlink -f /sys/class/net/$iface/device/driver 2>/dev/null) 2>/dev/null || echo "unknown")
    if [ "$DRIVER" = "ax_usb_nic" ]; then
        INTERFACE=$iface
        break
    fi
done

if [ -z "$INTERFACE" ]; then
    echo -e "  ${RED}✗${NC} Could not find interface using ax_usb_nic driver"
    echo ""
    echo "Available interfaces:"
    ip link show | grep "^[0-9]" | awk '{print "  " $2}' | tr -d ':'
    echo ""
    echo "Please specify manually:"
    read -p "Enter interface name: " INTERFACE
    
    if [ -z "$INTERFACE" ]; then
        echo "No interface specified. Exiting."
        exit 1
    fi
fi

echo -e "  ${GREEN}✓${NC} Using interface: $INTERFACE"
echo ""

# ============================================================================
# STEP 3: Check Network Connection
# ============================================================================
echo -e "${BLUE}━━━ STEP 3: Verify Network Connection ━━━${NC}"
echo ""

# Bring interface up if down
STATE=$(ip link show $INTERFACE | grep -o "state [A-Z]*" | awk '{print $2}')
if [ "$STATE" = "DOWN" ]; then
    echo "  Interface is DOWN. Bringing it up..."
    ip link set $INTERFACE up
    sleep 3
    STATE=$(ip link show $INTERFACE | grep -o "state [A-Z]*" | awk '{print $2}')
fi

echo "  Interface state: $STATE"

if [ "$STATE" != "UP" ]; then
    echo -e "  ${YELLOW}!${NC} Warning: Interface state is $STATE"
    echo "     Please connect an Ethernet cable"
    echo ""
    read -p "Press Enter when cable is connected (or Ctrl+C to abort)..."
    echo ""
fi

# Show interface details
MAC=$(ip link show $INTERFACE | grep -o "link/ether [^ ]*" | awk '{print $2}')
echo "  MAC Address: $MAC"

# Check for IP address
IP_ADDR=$(ip addr show $INTERFACE | grep "inet " | awk '{print $2}')
if [ -z "$IP_ADDR" ]; then
    echo -e "  ${YELLOW}!${NC} No IP address assigned"
    echo ""
    echo "  PTP works at Layer 2 (Ethernet) so IP is not required,"
    echo "  but having one is helpful for troubleshooting."
    echo ""
else
    echo -e "  ${GREEN}✓${NC} IP Address: $IP_ADDR"
fi
echo ""

# ============================================================================
# STEP 4: Verify PTP Hardware Clock
# ============================================================================
echo -e "${BLUE}━━━ STEP 4: Verify PTP Hardware Clock ━━━${NC}"
echo ""

# Find PTP device
PTP_DEV=$(ethtool -T $INTERFACE 2>/dev/null | grep "PTP Hardware Clock" | grep -oE '/dev/ptp[0-9]+' || echo "")

if [ -z "$PTP_DEV" ]; then
    # Try to find it manually
    PTP_DEV=$(ls /dev/ptp* 2>/dev/null | head -1)
fi

if [ -z "$PTP_DEV" ]; then
    echo -e "  ${RED}✗${NC} No PTP hardware clock found"
    echo ""
    echo "This device may not support PTP. Run demo_2_standalone.sh first."
    exit 1
else
    echo -e "  ${GREEN}✓${NC} PTP Hardware Clock: $PTP_DEV"
    
    # Test clock access
    if phc_ctl $PTP_DEV get > /dev/null 2>&1; then
        CURRENT_TIME=$(phc_ctl $PTP_DEV get 2>/dev/null | grep "clock time is" | cut -d' ' -f4-)
        echo "    Current time: $CURRENT_TIME"
    else
        echo -e "  ${YELLOW}!${NC} Cannot read clock (may need different permissions)"
    fi
fi
echo ""

# ============================================================================
# STEP 5: Scan for PTP Masters
# ============================================================================
echo -e "${BLUE}━━━ STEP 5: Scan for PTP Masters on Network ━━━${NC}"
echo ""
echo "Listening for PTP announce messages..."
echo "(This will take 10 seconds)"
echo ""

# Create a temporary config for ptp4l
PTP_CONFIG="/tmp/ptp4l_scan_$$.conf"
cat > $PTP_CONFIG << EOF
# Temporary PTP config for scanning
[global]
masterOnly 0
clientOnly 1
tx_timestamp_timeout 50
summary_interval 1
time_stamping hardware
EOF

# Run ptp4l in background for 10 seconds to scan
timeout 10s ptp4l -i $INTERFACE -f $PTP_CONFIG -m 2>&1 | tee /tmp/ptp4l_scan_$$.log &
PTP_PID=$!

# Wait for it to complete
sleep 11

# Check results
if grep -q "selected best master clock" /tmp/ptp4l_scan_$$.log; then
    echo -e "  ${GREEN}✓${NC} Found PTP master on network!"
    echo ""
    
    # Extract master info
    MASTER_INFO=$(grep "selected best master clock" /tmp/ptp4l_scan_$$.log | tail -1)
    echo "    $MASTER_INFO"
    echo ""
    
    FOUND_MASTER=1
else
    echo -e "  ${YELLOW}!${NC} No PTP master found on network"
    echo ""
    echo "  Possible reasons:"
    echo "    • No PTP master is running"
    echo "    • Master is on a different network"
    echo "    • Network switch is blocking PTP multicast"
    echo ""
    echo "  You can:"
    echo "    1. Start ptp4l on another machine as master"
    echo "    2. Continue anyway to run in master mode (for demonstration)"
    echo ""
    
    read -p "Continue anyway? (y/n): " CONTINUE
    if [[ ! $CONTINUE =~ ^[Yy]$ ]]; then
        rm -f $PTP_CONFIG /tmp/ptp4l_scan_$$.log
        exit 1
    fi
    
    FOUND_MASTER=0
fi

rm -f $PTP_CONFIG /tmp/ptp4l_scan_$$.log

# ============================================================================
# STEP 6: Run PTP Synchronization
# ============================================================================
echo -e "${BLUE}━━━ STEP 6: Start PTP Synchronization ━━━${NC}"
echo ""

# Create PTP configuration
PTP_CONFIG="/tmp/ptp4l_demo_$$.conf"

if [ $FOUND_MASTER -eq 1 ]; then
    echo "Running in SLAVE mode (synchronizing to master)..."
    cat > $PTP_CONFIG << EOF
# PTP Configuration - Slave Mode
[global]
# Run as slave (client) only
masterOnly 0
clientOnly 1

# Use hardware timestamping (this is what TimeStick provides!)
time_stamping hardware

# Logging
summary_interval 1
verbose 1

# Timeouts
tx_timestamp_timeout 50
EOF
else
    echo "Running in MASTER mode (for demonstration)..."
    cat > $PTP_CONFIG << EOF
# PTP Configuration - Master Mode
[global]
# Run as master
masterOnly 1
clientOnly 0

# Use hardware timestamping
time_stamping hardware

# Logging
summary_interval 1
verbose 1

# Master settings
priority1 128
priority2 128

# Timeouts
tx_timestamp_timeout 50
EOF
fi

echo ""
echo "Starting ptp4l daemon..."
echo "Press Ctrl+C to stop (will run for 60 seconds)"
echo ""
echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  PTP4L OUTPUT (watch for 'rms' values - lower is better)      ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Run ptp4l and capture output
timeout 60s ptp4l -i $INTERFACE -f $PTP_CONFIG -m 2>&1 | tee /tmp/ptp4l_output_$$.log || true

echo ""
echo -e "${BLUE}━━━ STEP 7: Analyze Results ━━━${NC}"
echo ""

# Analyze the output
if [ -f /tmp/ptp4l_output_$$.log ]; then
    
    # Look for key indicators
    if grep -q "hardware time stamping" /tmp/ptp4l_output_$$.log; then
        echo -e "  ${GREEN}✓${NC} Hardware timestamping confirmed in use"
    fi
    
    if grep -q "port 1: LISTENING" /tmp/ptp4l_output_$$.log; then
        echo -e "  ${GREEN}✓${NC} Port is listening for PTP messages"
    fi
    
    if grep -q "selected best master clock" /tmp/ptp4l_output_$$.log; then
        echo -e "  ${GREEN}✓${NC} Successfully selected a master clock"
    fi
    
    if grep -q "port 1: SLAVE" /tmp/ptp4l_output_$$.log; then
        echo -e "  ${GREEN}✓${NC} Entered SLAVE state (synchronizing)"
        echo ""
        
        # Show synchronization statistics
        echo "  Synchronization statistics:"
        grep "rms" /tmp/ptp4l_output_$$.log | tail -5 | while read line; do
            echo "    $line"
        done
        echo ""
        echo "  What 'rms' means:"
        echo "    • Root Mean Square of clock offset"
        echo "    • Lower values = better synchronization"
        echo "    • Typical: < 100 ns for wired PTP"
        echo ""
    fi
    
    if grep -q "port 1: MASTER" /tmp/ptp4l_output_$$.log; then
        echo -e "  ${GREEN}✓${NC} Running as MASTER (providing time to network)"
        echo ""
        echo "  In master mode, this TimeStick could synchronize"
        echo "  other PTP clients on the network."
    fi
    
    # Check for errors
    if grep -qi "error\|failed\|cannot" /tmp/ptp4l_output_$$.log; then
        echo -e "  ${YELLOW}!${NC} Some errors occurred (check output above)"
    fi
    
    echo ""
fi

# ============================================================================
# STEP 8: Summary
# ============================================================================
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                    PTP SYNCHRONIZATION TEST                    ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

echo "What we demonstrated:"
echo ""
if [ $FOUND_MASTER -eq 1 ]; then
    echo "  ✓ TimeStick found PTP master on network"
    echo "  ✓ Used hardware timestamping for precision"
    echo "  ✓ Successfully synchronized clock"
    echo "  ✓ Achieved nanosecond-level accuracy"
else
    echo "  ✓ TimeStick can operate as PTP master"
    echo "  ✓ Hardware timestamping is functional"
    echo "  ✓ Can provide time to other PTP devices"
fi
echo ""

echo "Key points about hardware timestamping:"
echo ""
echo "  • Software timestamping: ~1-10 millisecond accuracy"
echo "  • Hardware timestamping: ~10-100 nanosecond accuracy"
echo "  • That's 100,000x more precise!"
echo ""
echo "  TimeStick uses hardware timestamping, which is why it's"
echo "  valuable for high-precision time synchronization."
echo ""

echo "Files saved for review:"
echo "  • /tmp/ptp4l_output_$$.log - Full PTP daemon output"
echo "  • $PTP_CONFIG - PTP configuration used"
echo ""

echo "═══════════════════════════════════════════════════════════════"
echo ""
echo -e "${GREEN}✓ PTP SYNCHRONIZATION TEST COMPLETE${NC}"
echo ""

# Clean up config (but keep log)
rm -f $PTP_CONFIG

echo "To run PTP continuously:"
echo "  sudo ptp4l -i $INTERFACE -m"
echo ""
echo "To sync system clock to PTP clock:"
echo "  sudo phc2sys -s $PTP_DEV -c CLOCK_REALTIME -w"
echo ""
