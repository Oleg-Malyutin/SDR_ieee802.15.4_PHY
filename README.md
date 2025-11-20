# SDR_ieee802.15.4_PHY
Software Defined Radio (SDR) used to implement the IEEE 802.15.4 physical layer (PHY)
(This is just the beginning, and so far only a receiver and a simple transmitter for active scanning have been implemented.)

The project was created using the Qt5 framework.

Tested configurations:
OS: Linux Mint 22.1 Xia.
Processor: Intel© Core™ i5-8600 CPU @ 3.10GHz × 6

Supported devices:
1. PlutoSDR (RX , TX)
2. HackRF One (With custom firmware. See /device/hackrf_one/firmware/Readme.txt)
3. ...

Received signal parameters:
1. O-QPSK (IEEE 802.15.4 - 2003)
2. Parsing :
 - frame control
 - addressing
 - frame check sequence
3. Capture and show raw frame
4. UDP ZEPv2. Support of other Ethernet based hardware to analyze 802.15.4 (https://wiki.wireshark.org/IEEE_802.15.4)
5. Scan energy detect
6. Active scan
7. ...


If you find this project useful, consider donating:

[Donate via PayPal](https://www.paypal.com/donate?hosted_button_id=A4EMYB46V67WJ)

**Bitcoin**: `bc1qem3sk3gumc3u8h6dx0nmffy4272vj8sx9s750p`
