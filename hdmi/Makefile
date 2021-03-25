obj-m := xilinx-vphy.o xilinx-hdmi-tx.o

# in same cases only used to check against presence, i.e. >0, actual number is not used
ccflags-y := -DXPAR_XVPHY_NUM_INSTANCES=4
ccflags-y += -DXPAR_XV_HDMIRXSS_NUM_INSTANCES=4
ccflags-y += -DXPAR_XV_HDMITXSS_NUM_INSTANCES=4
ccflags-y += -DXPAR_XV_HDMIRX_NUM_INSTANCES=4
ccflags-y += -DXPAR_XV_HDMITX_NUM_INSTANCES=4
ccflags-y += -DXPAR_XVTC_NUM_INSTANCES=4

ccflags-y += -DXPAR_VPHY_0_TRANSCEIVER=5
ccflags-y += -DXPAR_VPHY_0_RX_PROTOCOL=1
ccflags-y += -DXPAR_VPHY_0_TX_PROTOCOL=1
ccflags-y += -DXPAR_XV_HDMIRX_0_DEVICE_ID=0
ccflags-y += -DXPAR_XV_HDMITX_0_DEVICE_ID=0

ccflags-y += -DXPAR_XHDCP_NUM_INSTANCES=4
ccflags-y += -DXPAR_XHDCP22_RX_NUM_INSTANCES=4
ccflags-y += -DXPAR_XHDCP22_TX_NUM_INSTANCES=4
ccflags-y += -DXPAR_XHDCP22_MMULT_NUM_INSTANCES=4
ccflags-y += -DXPAR_XHDCP22_CIPHER_NUM_INSTANCES=8
ccflags-y += -DXPAR_XHDCP22_RNG_NUM_INSTANCES=8
ccflags-y += -DXPAR_XTMRCTR_NUM_INSTANCES=16

ccflags-y += -I${src}/phy-xilinx-vphy

# the BSP code uses C99 style declarations. The kernel is on C89, but steps to C11 are underway.
ccflags-y += -Wno-declaration-after-statement
ccflags-y += -Wno-unused-function
ccflags-y += -Wno-unused-label
ccflags-y += -Wno-unused-value
ccflags-y += -Wno-unused-variable

xilinx-hdmi-tx-objs := dlnx_hdmi_tx.o

# the C API around the IP registers
CFLAGS_dlnx_hdmi_tx.o += -I$(src)/xilinx-hdmi-tx/
CFLAGS_dlnx_hdmi_tx.o += -I$(src)/include/ -Idrivers/gpu/drm/xilinx/

xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitx.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitx_sinit.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitx_intr.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitxss.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitxss_log.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitxss_coreinit.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xv_hdmitxss_hdcp.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xvtc.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xvtc_intr.o
xilinx-hdmi-tx-objs += xilinx-hdmi-tx/xvtc_sinit.o
xilinx-hdmi-tx-objs += xlnx_hdmitx_audio.o

# PHY (HDMI RX/TX)
xilinx-vphy-objs := phy-vphy.o

# the C API around the IP registers
CFLAGS_phy-vphy.o += -I$(src)/phy-xilinx-vphy/
CFLAGS_phy-vphy.o += -I$(src)/include/
xilinx-vphy-objs += phy-xilinx-vphy/xdebug.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_i.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_gthe4.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_mmcme4.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_hdmi.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_hdmi_intr.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_intr.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_log.o
xilinx-vphy-objs += phy-xilinx-vphy/xvphy_sinit.o
xilinx-vphy-objs += phy-xilinx-vphy/xvidc.o
xilinx-vphy-objs += phy-xilinx-vphy/xvidc_timings_table.o

#hdmi common
xilinx-vphy-objs += phy-xilinx-vphy/xv_hdmic.o
xilinx-vphy-objs += phy-xilinx-vphy/xv_hdmic_vsif.o
