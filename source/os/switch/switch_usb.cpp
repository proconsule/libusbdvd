#include "switch_usb.h"
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <switch.h>
#include <stdio.h>
#include "usbdvd_utils.h"

#define USB_SCSI_TRANSPARENT 0x06
#define USB_PROTOCOL_BULK_ONLY 0x50

#define USB_ATAPI_SUBCLASS 0x02

#define USB_POSTBUFFER_TIMEOUT	(u64)10000000000


#define USB_BUF_ALIGNMENT	0x1000
#define USB_TRANS_BUF_SIZE	0x100000

#define RETRY_MAX 5

	
enum usb_request_bot {
    USB_REQUEST_BOT_GET_MAX_LUN = 0xFE,
    USB_REQUEST_BOT_RESET       = 0xFF
};

enum usb_request_recipient {
    USB_RECIPIENT_DEVICE    = 0x00,
    USB_RECIPIENT_INTERFACE = 0x01,
    USB_RECIPIENT_ENDPOINT  = 0x02,
    USB_RECIPIENT_OTHER     = 0x03,
};

enum usb_request_type {
    USB_REQUEST_TYPE_STANDARD = (0x00 << 5),
    USB_REQUEST_TYPE_CLASS    = (0x01 << 5),
    USB_REQUEST_TYPE_VENDOR   = (0x02 << 5),
    USB_REQUEST_TYPE_RESERVED = (0x03 << 5),
};

CSWITCH_USB::~CSWITCH_USB(){
	
	usbHsEpClose(&endpoint_out);
    usbHsEpClose(&endpoint_in);
    usbHsIfClose(&inf_session);

	
	usbHsDestroyInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent, 0);
	if(xfer_buf){
		free(xfer_buf);
		xfer_buf = NULL;
	}
	
	usbHsExit();
	
}

CSWITCH_USB::CSWITCH_USB(){
	
	s32 epi;
	struct usb_endpoint_descriptor *ep_desc = NULL;
	
	bool e_in = false;
	bool e_out = false;
	
	
	memset(&dvd_filter, 0, sizeof(dvd_filter));
    memset(&dvd_usbinterfaces, 8, sizeof(dvd_usbinterfaces));
	memset(&endpoint_out,0,sizeof(UsbHsClientEpSession));
	memset(&endpoint_in,0,sizeof(UsbHsClientEpSession));
	
	dvd_filter.Flags =  (UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass);
    dvd_filter.bInterfaceClass = USB_CLASS_MASS_STORAGE;
	dvd_filter.bInterfaceSubClass = USB_ATAPI_SUBCLASS;
	
	
	this->xfer_buf = (uint8_t *)memalign(USB_BUF_ALIGNMENT, USB_TRANS_BUF_SIZE);
	
	if(!xfer_buf)return;
	
	int rc = usbHsInitialize();
	if (R_FAILED(rc)){
		return;
	}
	
	rc = usbHsCreateInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent, true, 0, &dvd_filter);
	if (R_FAILED(rc)){
		return;
	}
	
	Waiter usb_if_available_waiter = waiterForEvent(&dvd_usbInterfaceAvailableEvent);
   
	
	rc = waitSingle(usb_if_available_waiter, 1000000000L);
	
	
	int total_entries = 0;
	memset(dvd_usbinterfaces, 0, sizeof(dvd_usbinterfaces));
	rc = usbHsQueryAvailableInterfaces(&dvd_filter, dvd_usbinterfaces, sizeof(dvd_usbinterfaces), &total_entries);
    if (R_FAILED(rc)){
		return;
	} 
	for(int i=0; i<total_entries; i++) {
		UsbHsClientIfSession *usb_if_session = &(inf_session);
		rc = usbHsAcquireUsbIf(usb_if_session, &dvd_usbinterfaces[i]);
		if (R_SUCCEEDED(rc)) {
			if (xfer_buf) {
				u32 transferredSize=0;
				memset(xfer_buf,0,USB_TRANS_BUF_SIZE);
				rc = usbHsIfCtrlXfer(usb_if_session, USB_ENDPOINT_IN, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_CONFIG<<8) | 0, 0, 0x40, xfer_buf, &transferredSize);
				//usbdvd_log("usbHsIfCtrlXfer(interface index = %d) returned: 0x%x, transferredSize=0x%x\n", i, rc, transferredSize);
				for(epi=0; epi<15; epi++) {
					ep_desc = &usb_if_session->inf.inf.output_endpoint_descs[epi];
					if (ep_desc->bLength != 0 && !(ep_desc->bEndpointAddress & USB_ENDPOINT_IN)) {
						//usbdvd_log("Using OUTPUT endpoint %d.\n", epi);

						rc = usbHsIfOpenUsbEp(usb_if_session, &endpoint_out, 1, ep_desc->wMaxPacketSize, ep_desc);
						//usbdvd_log("usbHsIfOpenUsbEp returned: 0x%x\n", rc);
						e_out = true;
						if (R_FAILED(rc)) break;
					}
					ep_desc = &usb_if_session->inf.inf.input_endpoint_descs[epi];
					if (ep_desc->bLength != 0 && (ep_desc->bEndpointAddress & USB_ENDPOINT_IN)) {
						//usbdvd_log("Using INPUT endpoint %d.\n", epi);

						e_in = true;
						rc = usbHsIfOpenUsbEp(usb_if_session, &endpoint_in, 1, ep_desc->wMaxPacketSize, ep_desc);
						//usbdvd_log("usbHsIfOpenUsbEp returned: 0x%x\n", rc);
						if (R_FAILED(rc)) break;
					}

					if (e_in && e_out) break;
				}
			}
		}
		
		if (e_in && e_out){

			usb_clear_halt(usb_if_session, &endpoint_out);
			usb_clear_halt(usb_if_session, &endpoint_in);
			usbdvd_log("USB INIT OK, ENDPOINT OK\r\n");
			device_found = true;
			
			return;
		}
		
		
		
	}
	
}



int CSWITCH_USB::usb_bulk_transfer(bool receive,void *buf,uint32_t data_size,uint32_t  *data_transfered,uint32_t timeout){
	
	auto lk = std::scoped_lock(usb_mutex);
	
	if(!receive){
		memcpy(xfer_buf,buf,data_size);
	}
	Result rc = -1;
	int i = 0;
    	do {
            rc = usbRequestTransfer(receive ? &endpoint_in : &endpoint_out, (void *)xfer_buf, data_size, data_transfered,timeout*100000);
            if (R_FAILED(rc)) {
				usb_clear_halt(&inf_session,receive ? &endpoint_in:&endpoint_out);
            }
            i++;
	} while ((rc != 0) && (i<RETRY_MAX));
	
	if(receive && R_SUCCEEDED(rc)){
		memcpy(buf,xfer_buf,data_size);
	}
	return rc;
	
}

/* USB TRANSFER and HALT taken from the great libusbhsfs https://github.com/DarkMatterCore/libusbhsfs */

Result CSWITCH_USB::usbRequestTransfer(UsbHsClientEpSession *usb_ep_session, void *buf, u32 size, u32 *xfer_size,u32 timeout){
	Result rc = 0;
	Event *xfer_event = NULL;
    u32 xfer_id = 0;
	UsbHsXferReport report = {0};
    u32 report_count = 0;
	if (!usb_ep_session || !serviceIsActive(&(usb_ep_session->s)) || !buf || !size || !xfer_size)
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        goto end;
    }

    *xfer_size = 0;
	 /* Get endpoint transfer event. */
    xfer_event = usbHsEpGetXferEvent(usb_ep_session);

    /* Perform asynchronous USB data transfer. */
    rc = usbHsEpPostBufferAsync(usb_ep_session, buf, size, 0, &xfer_id);
    if (R_FAILED(rc))
    {
        goto end;
    }

    /* Wait until USB data transfer is complete. */
    /* TODO: find a way to properly cancel an async transfer. If left unhandled, this may trigger a fatal error within the usb sysmodule. */
    rc = eventWait(xfer_event, USB_POSTBUFFER_TIMEOUT);
    if (R_SUCCEEDED(rc) || R_VALUE(rc) == KERNELRESULT(TimedOut)) eventClear(xfer_event);

    if (R_FAILED(rc))
    {
        goto end;
    }

    /* Retrieve USB transfer report. */
    rc = usbHsEpGetXferReport(usb_ep_session, &report, 1, &report_count);
    if (R_FAILED(rc))
    {
        goto end;
    }

    if (report_count < 1)
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        goto end;
    }

    /* Save transferred data size. */
    *xfer_size = report.transferredSize;

    /* Update return value. */
    rc = report.res;

end:
    return rc;
	
}

Result CSWITCH_USB::usb_device_reset(){
	
	
	UsbHsClientIfSession *usb_if_session = &inf_session;
	
	Result rc = 0;
    u16 if_num = 0;
    u32 xfer_size = 0;

    if (!usb_if_session || !usbHsIfIsActive(usb_if_session))
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        goto end;
    }

    if_num = usb_if_session->inf.inf.interface_desc.bInterfaceNumber;

    /* Perform control transfer. */
    rc = usbHsIfCtrlXfer(usb_if_session, USB_ENDPOINT_OUT | USB_REQUEST_TYPE_CLASS | USB_RECIPIENT_INTERFACE, USB_REQUEST_BOT_RESET, 0, if_num, 0, NULL, &xfer_size);
    
end:
    return rc;
	
}



int CSWITCH_USB::usb_clear_halt(UsbHsClientIfSession *usb_if_session,UsbHsClientEpSession *usb_ep_session){
	
	
	Result rc = 0;
    u16 ep_addr = 0;
    u32 xfer_size = 0;

    if (!usb_if_session || !usbHsIfIsActive(usb_if_session) || !usb_ep_session || !serviceIsActive(&(usb_ep_session->s)))
    {
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        goto end;
    }

    ep_addr = usb_ep_session->desc.bEndpointAddress;

    /* Perform control transfer. */
    rc = usbHsIfCtrlXfer(usb_if_session, USB_ENDPOINT_OUT | USB_REQUEST_TYPE_STANDARD | USB_RECIPIENT_ENDPOINT, USB_REQUEST_CLEAR_FEATURE, 0x00, ep_addr, 0, NULL, &xfer_size);
    
end:
    return rc;
	
	
}