# Pinout

| Signal | Pin | Notes |
| --- | --- | --- |
| UART7 RX | PE7 AF7 | G0 / LDO, 460800 8N1 |
| UART7 TX | PE8 AF7 | G0 / LDO |
| LD1 | PO1 | Existing firmware writes high for on. Left unchanged. |
| USER button B2 | PC13 | UM3289: pressed = high, released = low. Internal pulldown. Polled, not EXTI. Zephyr calls it active-low; verify on the bench before trusting the edge. |
| LCD enable | PE15 | Existing |
| Touch IRQ | PE3 | Existing |
| Backlight | PG15 | Existing |
| USART1 G4 | not assigned | 115200 8N1 is specified. Pins are not in this repo or the ioc, so the peripheral is left off. |
| Ethernet | not in Appli | The DK has an Ethernet PHY. The Appli project does not enable the ETH IP. |

B1 on this board is reset, not the user button.
