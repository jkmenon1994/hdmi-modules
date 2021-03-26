#ifndef DLNX_HDMI_DRIVER_API_H
#define DLNX_HDMI_DRIVER_API_H

/* @sdi_mode: configurable SDI mode parameter, supported values are:
 *              0 - HD
 *              1 - SD
 *              2 - 3GA
 *              3 - 3GB
 *              4 - 6G
 *              5 - 12G
 *
 * @sdi_data_stream: configurable number of SDI data streams
 *                          value currently supported are 2, 4 and 8
 */

struct hdmi_output_params {
	int hdisplay;
	int vdisplay;
	int vrefresh;
};

#define DLNX_IOC_MAGIC  	'D'
#define DLNX_HDMI_SET_MODE 	_IOWR(DLNX_IOC_MAGIC, 1, struct hdmi_output_params)

#endif
