

copy pl-crtc-driver/xlnx_pl_disp.c to <project-root>/build/tmp/work-shared/plnx-zynqmp/kernel-source/drivers/gpu/drm/xlnx/

copy clk-si5341.c Kconfig Makefile to  <project-root>/build/tmp/work-shared/plnx-zynqmp/kernel-source/drivers/ 

petalinux-config -c kernel

select the si5341/45-driver from Device-drivers--> Common-clock-framework

petalinux-build
