/*
 * ASIX AX88279/AX88179 Hardware Definitions
 * Ported from Linux driver ax_main.h
 */

#ifndef _ASIX_HW_H_
#define _ASIX_HW_H_

// USB Vendor and Product IDs
#define USB_VENDOR_ID_ASIX              0x0B95
#define USB_PRODUCT_ID_AX88179          0x1790  // bcdDevice 0x0100
#define USB_PRODUCT_ID_AX88179A         0x1790  // bcdDevice 0x0200
#define USB_PRODUCT_ID_AX88178A         0x178A  // bcdDevice 0x0100
#define USB_PRODUCT_ID_AX88279          0x1790  // bcdDevice 0x0400

// ASIX Command Types
#define AX_ACCESS_MAC                   0x01
#define AX_ACCESS_PHY                   0x02
#define AX_ACCESS_WAKEUP                0x03
#define AX_ACCESS_EEPROM                0x04
#define AX_ACCESS_EFUSE                 0x05
#define AX_RELOAD_EEPROM_EFUSE          0x06
#define AX_WRITE_EFUSE_EN               0x09
#define AX_WRITE_EFUSE_DIS              0x0A
#define AX_ACCESS_MFAB                  0x10

// Register Addresses
#define AX_PHYSICAL_LINK_STATUS         0x02
    #define AX_USB_SS                       0x04  // USB 3.0
    #define AX_USB_HS                       0x02  // USB 2.0 High Speed
    #define AX_USB_FS                       0x01  // USB 2.0 Full Speed

#define AX_GENERAL_STATUS               0x03
    #define AX_SECLD                        0x04

#define AX_SROM_ADDR                    0x07
#define AX_SROM_CMD                     0x0a
    #define AX_EEP_RD                       0x04
    #define AX_EEP_WR                       0x08
    #define AX_EEP_BUSY                     0x10

#define AX_SROM_DATA_LOW                0x08
#define AX_SROM_DATA_HIGH               0x09

#define AX_RX_CTL                       0x0b
    #define AX_RX_CTL_DROPCRCERR            0x0100
    #define AX_RX_CTL_IPE                   0x0200
    #define AX_RX_CTL_START                 0x0080
    #define AX_RX_CTL_AP                    0x0020  // Accept Physical
    #define AX_RX_CTL_AM                    0x0010  // Accept Multicast
    #define AX_RX_CTL_AB                    0x0008  // Accept Broadcast
    #define AX_RX_CTL_AMALL                 0x0002  // Accept All Multicast
    #define AX_RX_CTL_PRO                   0x0001  // Promiscuous
    #define AX_RX_CTL_STOP                  0x0000

#define AX_NODE_ID                      0x10  // MAC address (6 bytes)
#define AX_MULTI_FILTER_ARRY            0x16  // Multicast filter (8 bytes)

#define AX_MEDIUM_STATUS_MODE           0x22
    #define AX_MEDIUM_GIGAMODE              0x0001
    #define AX_MEDIUM_FULL_DUPLEX           0x0002
    #define AX_MEDIUM_ALWAYS_ONE            0x0004
    #define AX_MEDIUM_RXFLOW_CTRLEN         0x0010
    #define AX_MEDIUM_TXFLOW_CTRLEN         0x0020
    #define AX_MEDIUM_RECEIVE_EN            0x0100
    #define AX_MEDIUM_PS                    0x0200  // Port speed (100M)
    #define AX_MEDIUM_JUMBO_EN              0x8040

#define AX_MONITOR_MODE                 0x24
    #define AX_MONITOR_MODE_RWLC            0x02
    #define AX_MONITOR_MODE_RWMP            0x04
    #define AX_MONITOR_MODE_PMEPOL          0x20
    #define AX_MONITOR_MODE_PMETYPE         0x40

#define AX_GPIO_CTRL                    0x25
    #define AX_GPIO_CTRL_GPIO3EN            0x80
    #define AX_GPIO_CTRL_GPIO2EN            0x40
    #define AX_GPIO_CTRL_GPIO1EN            0x20

#define AX_PHYPWR_RSTCTL                0x26
    #define AX_PHYPWR_RSTCTL_BZ             0x0010
    #define AX_PHYPWR_RSTCTL_IPRL           0x0020
    #define AX_PHYPWR_RSTCTL_AUTODETACH     0x1000

#define AX_RX_BULKIN_QCTRL              0x2e
    #define AX_RX_BULKIN_QCTRL_TIME         0x01
    #define AX_RX_BULKIN_QCTRL_IFG          0x02
    #define AX_RX_BULKIN_QCTRL_SIZE         0x04

#define AX_RX_BULKIN_QTIMR_LOW          0x2f
#define AX_RX_BULKIN_QTIMR_HIGH         0x30
#define AX_RX_BULKIN_QSIZE              0x31
#define AX_RX_BULKIN_QIFG               0x32

#define AX_CLK_SELECT                   0x33
    #define AX_CLK_SELECT_BCS               0x01
    #define AX_CLK_SELECT_ACS               0x02
    #define AX_CLK_SELECT_ACSREQ            0x10
    #define AX_CLK_SELECT_ULR               0x08

#define AX_RXCOE_CTL                    0x34
    #define AX_RXCOE_IP                     0x01
    #define AX_RXCOE_TCP                    0x02
    #define AX_RXCOE_UDP                    0x04
    #define AX_RXCOE_ICMP                   0x08
    #define AX_RXCOE_IGMP                   0x10
    #define AX_RXCOE_TCPV6                  0x20
    #define AX_RXCOE_UDPV6                  0x40
    #define AX_RXCOE_ICMV6                  0x80

#define AX_TXCOE_CTL                    0x35
    #define AX_TXCOE_IP                     0x01
    #define AX_TXCOE_TCP                    0x02
    #define AX_TXCOE_UDP                    0x04
    #define AX_TXCOE_ICMP                   0x08
    #define AX_TXCOE_IGMP                   0x10
    #define AX_TXCOE_TCPV6                  0x20
    #define AX_TXCOE_UDPV6                  0x40
    #define AX_TXCOE_ICMV6                  0x80

#define AX_PAUSE_WATERLVL_HIGH          0x54
#define AX_PAUSE_WATERLVL_LOW           0x55

// PHY (GMII) Register Addresses
#define GMII_PHY_CONTROL                0x00
    #define GMII_CONTROL_RESET              0x8000
    #define GMII_CONTROL_LOOPBACK           0x4000
    #define GMII_CONTROL_100MB              0x2000
    #define GMII_CONTROL_1000MB             0x0040
    #define GMII_CONTROL_ENABLE_AUTO        0x1000
    #define GMII_CONTROL_POWER_DOWN         0x0800
    #define GMII_CONTROL_START_AUTO         0x0200
    #define GMII_CONTROL_FULL_DUPLEX        0x0100

#define GMII_PHY_STATUS                 0x01
    #define GMII_STATUS_AUTO_DONE           0x0020
    #define GMII_STATUS_LINK_UP             0x0004

#define GMII_PHY_ANAR                   0x04
    #define GMII_ANAR_PAUSE                 0x0400
    #define GMII_ANAR_100TXFD               0x0100
    #define GMII_ANAR_100TX                 0x0080
    #define GMII_ANAR_10TFD                 0x0040
    #define GMII_ANAR_10T                   0x0020

#define GMII_PHY_1000BT_CONTROL         0x09
#define GMII_PHY_1000BT_STATUS          0x0A

#define GMII_PHY_PHYSR                  0x11
    #define GMII_PHY_PHYSR_GIGA             0x8000
    #define GMII_PHY_PHYSR_100              0x4000
    #define GMII_PHY_PHYSR_FULL             0x2000
    #define GMII_PHY_PHYSR_LINK             0x0400

// Packet Header Definitions
#define AX_TX_HEADER_SIZE               8
#define AX_RX_HEADER_SIZE               4

// TX Header Flags
#define AX_TX_HDR_PADDING               0x80000000

// RX Header Flags
#define AX_RXHDR_CRC_ERR                0x20000000
#define AX_RXHDR_MII_ERR                0x40000000
#define AX_RXHDR_DROP_ERR               0x80000000
#define AX_RXHDR_L4_ERR                 (1 << 8)
#define AX_RXHDR_L3_ERR                 (1 << 9)

// Buffer Sizes
#define AX_MAX_MCAST                    64
#define AX_MCAST_FILTER_SIZE            8
#define AX_RX_URB_SIZE                  (24 * 1024)  // 24KB per URB
#define AX_TX_TIMEOUT_MS                5000

// Link Speed Values
typedef enum _AX_LINK_SPEED {
    AX_LINK_SPEED_UNKNOWN = 0,
    AX_LINK_SPEED_10MBPS = 1,
    AX_LINK_SPEED_100MBPS = 2,
    AX_LINK_SPEED_1000MBPS = 3
} AX_LINK_SPEED;

// Chip Versions
typedef enum _AX_CHIP_VERSION {
    AX_VERSION_INVALID = 0,
    AX_VERSION_AX88179 = 4,
    AX_VERSION_AX88179A = 6,
    AX_VERSION_AX88279 = 7
} AX_CHIP_VERSION;

// USB Control Request Format
#define AX_USB_CTRL_READ                0xC0  // Device to Host
#define AX_USB_CTRL_WRITE               0x40  // Host to Device

// Helper Macros
#define AX_MAKE_WORD(low, high)         ((USHORT)(((UCHAR)(low)) | ((USHORT)((UCHAR)(high))) << 8))
#define AX_LOBYTE(w)                    ((UCHAR)(w))
#define AX_HIBYTE(w)                    ((UCHAR)(((USHORT)(w) >> 8) & 0xFF))

#endif // _ASIX_HW_H_
