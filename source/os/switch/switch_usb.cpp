#include "switch_usb.h"
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <switch.h>
#include <stdio.h>
#include "usbdvd_utils.h"
#include "usbdvd_scsi_common.h"

#define USB_SCSI_TRANSPARENT 0x06
#define USB_PROTOCOL_BULK_ONLY 0x50

#define USB_ATAPI_SUBCLASS 0x02
#define USB_TRANSPARENT_CMD_SET_SUBCLASS 0x06

#define USB_POSTBUFFER_TIMEOUT  (u64)10000000000


#define USB_BUF_ALIGNMENT   0x1000


#define RETRY_MAX 3

    
enum usb_request_bot {
    USB_REQUEST_BOT_GET_MAX_LUN = 0xFE,
    USB_REQUEST_BOT_RESET       = 0xFF
};

/*

enum usb_request_recipient {
    USB_RECIPIENT_DEVICE    = 0x00,
    USB_RECIPIENT_INTERFACE = 0x01,
    USB_RECIPIENT_ENDPOINT  = 0x02,
    USB_RECIPIENT_OTHER     = 0x03,
};

*/

/*

enum usb_request_type {
    USB_REQUEST_TYPE_STANDARD = (0x00 << 5),
    USB_REQUEST_TYPE_CLASS    = (0x01 << 5),
    USB_REQUEST_TYPE_VENDOR   = (0x02 << 5),
    USB_REQUEST_TYPE_RESERVED = (0x03 << 5),
};

*/

CSWITCH_USB::~CSWITCH_USB(){
    
    usbHsEpClose(&endpoint_out);
    usbHsEpClose(&endpoint_in);
    usbHsIfClose(&inf_session);

    
    usbHsDestroyInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent_sub02, 0);
    usbHsDestroyInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent_sub06, 0);
    
    
    if(xfer_buf){
        free(xfer_buf);
        xfer_buf = NULL;
    }
    
    usbHsExit();
    
}

CSWITCH_USB::CSWITCH_USB(bool _unfilter_subclass6,bool verboseinit){
    
    unfilter_subclass6 = _unfilter_subclass6;
    s32 epi;
    struct usb_endpoint_descriptor *ep_desc = NULL;
    
    bool e_in = false;
    bool e_out = false;
	bool if_session_open = false;
    int total_entries_sub02 = 0;
    int total_entries_sub06 = 0;
    int total_entries = 0;
    Waiter usb_if_available_waiter_sub02;
    Waiter usb_if_available_waiter_sub06;
    
    
    memset(&dvd_filter_sub02, 0, sizeof(dvd_filter_sub02));
    memset(&dvd_usbinterfaces_sub02, 8, sizeof(dvd_usbinterfaces_sub02));
    memset(&dvd_filter_sub06, 0, sizeof(dvd_filter_sub06));
    memset(&dvd_usbinterfaces_sub06, 8, sizeof(dvd_usbinterfaces_sub06));
    
    memset(&endpoint_out,0,sizeof(UsbHsClientEpSession));
    memset(&endpoint_in,0,sizeof(UsbHsClientEpSession));
    
    dvd_filter_sub02.Flags =  (UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass);
    dvd_filter_sub02.bInterfaceClass = USB_CLASS_MASS_STORAGE;
    dvd_filter_sub02.bInterfaceSubClass = USB_ATAPI_SUBCLASS;
    
    
    if(unfilter_subclass6){
        dvd_filter_sub06.Flags =  (UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass);
        dvd_filter_sub06.bInterfaceClass = USB_CLASS_MASS_STORAGE;
        dvd_filter_sub06.bInterfaceSubClass = USB_SCSI_TRANSPARENT;
    }
    
    if(verboseinit){
        if(unfilter_subclass6){
            printf("Init device filter for subclass: %02hhx %02hhx\n",dvd_filter_sub02.bInterfaceSubClass,dvd_filter_sub06.bInterfaceSubClass);
        }else{
            printf("Init device filter for subclass: %02hhx\n",dvd_filter_sub02.bInterfaceSubClass);
        }
        
    }
    this->xfer_buf = (uint8_t *)memalign(USB_BUF_ALIGNMENT, USB_TRANS_BUF_SIZE);
    
    if(!xfer_buf){
        return;
    }
    int rc = usbHsInitialize();
    if (R_FAILED(rc)){
        goto cleanup_mem;
    }
    
    rc = usbHsCreateInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent_sub02, true, 0, &dvd_filter_sub02);
    if (R_FAILED(rc)){
        goto cleanup_usbhs;
    }
    
    
    usb_if_available_waiter_sub02 = waiterForEvent(&dvd_usbInterfaceAvailableEvent_sub02);
   

   
    if(verboseinit){
        printf("Wait for devices.....\n");
    }
    rc = waitSingle(usb_if_available_waiter_sub02, 1000000000L);
    if (R_FAILED(rc)) {
        if(!unfilter_subclass6){
            goto cleanup_event;
        }
    }else{
    
        memset(dvd_usbinterfaces_sub02, 0, sizeof(dvd_usbinterfaces_sub02));
        rc = usbHsQueryAvailableInterfaces(&dvd_filter_sub02, dvd_usbinterfaces_sub02, sizeof(dvd_usbinterfaces_sub02), &total_entries_sub02);
        if (R_FAILED(rc)){
            if(verboseinit){
                printf("NO USB DEVICES FOUND with subclass: %02hhx\n",dvd_filter_sub02.bInterfaceSubClass);
            }
            if(!unfilter_subclass6){
                goto cleanup_event;
            }
        }
        
        if(total_entries_sub02>0){
            total_entries = total_entries_sub02;
            subclass_used = USB_ATAPI_SUBCLASS;
            memcpy(dvd_usbinterfaces,dvd_usbinterfaces_sub02,sizeof(dvd_usbinterfaces));
        }
    
    }
    
    if(total_entries==0 && unfilter_subclass6){
        
        
        
        rc = usbHsCreateInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent_sub06, true, 0, &dvd_filter_sub06);
        if (R_FAILED(rc)){
            goto cleanup_usbhs;
        }
       
            
        usb_if_available_waiter_sub06 = waiterForEvent(&dvd_usbInterfaceAvailableEvent_sub06);
    
        
        
        if(verboseinit){
            printf("Wait for devices subclass 0x06.....\n");
        }
   
        rc = waitSingle(usb_if_available_waiter_sub06, 1000000000L);
        if (R_FAILED(rc)) {
            goto cleanup_event; // Salta alla pulizia dell'evento
        }
        
        memset(dvd_usbinterfaces_sub06, 0, sizeof(dvd_usbinterfaces_sub06));
        rc = usbHsQueryAvailableInterfaces(&dvd_filter_sub06, dvd_usbinterfaces_sub06, sizeof(dvd_usbinterfaces_sub06), &total_entries_sub06);
        if (R_FAILED(rc)){
            if(verboseinit){
                printf("NO USB DEVICES FOUND with subclass: %02hhx\n",dvd_filter_sub06.bInterfaceSubClass);
            }
            
            goto cleanup_event;
            
        }
    
        if(total_entries_sub06>0){
            subclass_used = USB_TRANSPARENT_CMD_SET_SUBCLASS;
            total_entries = total_entries_sub06;
            memcpy(dvd_usbinterfaces,dvd_usbinterfaces_sub06,sizeof(dvd_usbinterfaces));
        }
    
    
    }
    
    if(verboseinit){
        printf("FOUND %d devices\r\n",total_entries);
    }
    for(int i=0; i<total_entries; i++) {
        UsbHsClientIfSession *usb_if_session = &(inf_session);
        selsubclass = usb_if_session->inf.device_desc.bDeviceSubClass;
        rc = usbHsAcquireUsbIf(usb_if_session, &dvd_usbinterfaces[i]);
        
        if (R_SUCCEEDED(rc)) {
			if_session_open = true;
            if (xfer_buf) {
                u32 transferredSize=0;
                memset(xfer_buf,0,USB_TRANS_BUF_SIZE);
                rc = usbHsIfCtrlXfer(usb_if_session, USB_ENDPOINT_IN, USB_REQUEST_GET_DESCRIPTOR, (USB_DT_CONFIG<<8) | 0, 0, 0x40, xfer_buf, &transferredSize);
                //usbdvd_log("usbHsIfCtrlXfer(interface index = %d) returned: 0x%x, transferredSize=0x%x\n", i, rc, transferredSize);
                printf("USB Interface bInterfaceProtocol %02hhx\r\n",usb_if_session->inf.inf.interface_desc.bInterfaceProtocol);
        
                if(verboseinit){
                    printf("Searching for endpoints\r\n");
                }
                for(epi=0; epi<15; epi++) {
                    ep_desc = &usb_if_session->inf.inf.output_endpoint_descs[epi];
                    if (ep_desc->bLength != 0 && !(ep_desc->bEndpointAddress & USB_ENDPOINT_IN)) {
                        //usbdvd_log("Using OUTPUT endpoint %d.\n", epi);
                        if(verboseinit){
                            printf("Using OUTPUT endpoint %d\n", epi);
                        }
                        rc = usbHsIfOpenUsbEp(usb_if_session, &endpoint_out, 1, ep_desc->wMaxPacketSize, ep_desc);
                        //usbdvd_log("usbHsIfOpenUsbEp returned: 0x%x\n", rc);
                        if (R_FAILED(rc)) break;
						e_out = true;
                    }
                    ep_desc = &usb_if_session->inf.inf.input_endpoint_descs[epi];
                    if (ep_desc->bLength != 0 && (ep_desc->bEndpointAddress & USB_ENDPOINT_IN)) {
                        //usbdvd_log("Using INPUT endpoint %d.\n", epi);
                        if(verboseinit){
                            printf("Using INPUT endpoint %d\n", epi);
                        }
                        rc = usbHsIfOpenUsbEp(usb_if_session, &endpoint_in, 1, ep_desc->wMaxPacketSize, ep_desc);
                        //usbdvd_log("usbHsIfOpenUsbEp returned: 0x%x\n", rc);
                        if (R_FAILED(rc)) break;
						e_in = true;
                    }

                    if (e_in && e_out) break;
                }
            }
        }
        
        if (e_in && e_out){
            
            //subclass_used = unfilter_subclass6 ? USB_SCSI_TRANSPARENT:USB_ATAPI_SUBCLASS;
            usb_clear_halt(usb_if_session, &endpoint_out);
            usb_clear_halt(usb_if_session, &endpoint_in);
            usbdvd_log("USB INIT OK, ENDPOINT OK\r\n");
            
            //device_found = true;
            device_found = check_is_optical();
            
			if(!device_found){

				usbHsEpClose(&endpoint_out);
				usbHsEpClose(&endpoint_in);
				usbHsIfClose(usb_if_session);
				e_in = false;
				e_out = false;
				if_session_open = false;
				continue;
			}
            
            
            if(verboseinit){
                printf("USB INIT OK, ENDPOINT OK\n");
            }
            return;
        }else{
            if(verboseinit){
                printf("UNABLE TO FIND IN/OUT ENDPOINT\n");
            }
           
            goto cleanup_if; // Salta alla pulizia dell'interfaccia
           
        }
        
        
        
    }
    if(total_entries==0){
        if(verboseinit){
            if(unfilter_subclass6){
                printf("NO USB DEVICES FOUND with subclass: %02hhx %02hhx\n",dvd_filter_sub02.bInterfaceSubClass,dvd_filter_sub06.bInterfaceSubClass);
            }else{
                printf("NO USB DEVICES FOUND with subclass: %02hhx\n",dvd_filter_sub02.bInterfaceSubClass);
            }
        }
        
    }
    
    
    
    cleanup_if:
		if(if_session_open) usbHsIfClose(&inf_session);
    cleanup_event:
        usbHsDestroyInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent_sub02, 0);
        usbHsDestroyInterfaceAvailableEvent(&dvd_usbInterfaceAvailableEvent_sub06, 0);
    cleanup_usbhs:
        usbHsExit();
    cleanup_mem:
        free(xfer_buf);
        xfer_buf = NULL;
        return;
    
    
}



int CSWITCH_USB::usb_bulk_transfer(bool receive,void *buf,uint32_t data_size,uint32_t  *data_transfered,uint32_t timeout){
    
    
    auto lk = std::scoped_lock(usb_mutex);
    
    if (!buf || !xfer_buf || !data_transfered) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }
	
	if (data_size > USB_TRANS_BUF_SIZE) {

		usbdvd_log("usb_bulk_transfer: data_size %u exceeds xfer_buf size %u\r\n",
				data_size, (uint32_t)USB_TRANS_BUF_SIZE);
		return MAKERESULT(Module_Libnx, LibnxError_BadInput);
	}
	
    if(!receive){
        memcpy(xfer_buf,buf,data_size);
    }
    Result rc = -1;
    //int i = 0;
    //    do {
            rc = usbRequestTransfer(receive ? &endpoint_in : &endpoint_out, (void *)xfer_buf, data_size, data_transfered,timeout);
            if (R_FAILED(rc)) {
                usb_clear_halt(&inf_session,receive ? &endpoint_in:&endpoint_out);
            }
    //        i++;
    //} while ((rc != 0) && (i<RETRY_MAX));
	
	
    
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
    rc = usbHsIfCtrlXfer(usb_if_session, (u8)USB_ENDPOINT_OUT | (u8)USB_REQUEST_TYPE_CLASS | (u8)USB_RECIPIENT_INTERFACE, (u8)USB_REQUEST_BOT_RESET, 0, if_num, 0, NULL, &xfer_size);
    
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
    rc = usbHsIfCtrlXfer(usb_if_session, (u8)USB_ENDPOINT_OUT | (u8)USB_REQUEST_TYPE_STANDARD | (u8)USB_RECIPIENT_ENDPOINT, (u8)USB_REQUEST_CLEAR_FEATURE, 0x00, ep_addr, 0, NULL, &xfer_size);
    
end:
    return rc;
    
    
}

void CSWITCH_USB::ResetEndpoints(){
    
    usb_clear_halt(&inf_session,&endpoint_in);
    usb_clear_halt(&inf_session,&endpoint_out);
    
}

bool CSWITCH_USB::check_is_optical(){
    
    uint8_t buf[5];
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,5,true,0,6);
    
    uint16_t allocation_length_bigendian = __builtin_bswap16(5);
    cbw.CBWCB[0] = 0x12;            
    cbw.CBWCB[1] = 0;                 
    cbw.CBWCB[2] = 0;
    memcpy(&(cbw.CBWCB[3]), &allocation_length_bigendian, sizeof(uint16_t));
    int r=0;
    
    uint32_t transferred = 0;
    
    usb_bulk_transfer(false, &cbw, sizeof(CBW), &transferred, 5000);
    if (r < 0) {
        
    }
    
   
    r = usb_bulk_transfer(true, buf, cbw.dCBWDataTransferLength, &transferred, 10000);
        
    if (r < 0) {
       
        }
        
    
    // Ricevi CSW
    uint32_t cswtrans = 0;
    CSW csw{0};
    r = usb_bulk_transfer(true, (uint8_t*)&csw, sizeof(CSW), &cswtrans, 5000);
    
    if(cswtrans == sizeof(CSW)) {
        if (r < 0) {
                return r;
            }

            
            if (csw.dCSWSignature != CSW_SIGNATURE) {
                usbdvd_log("SIG ERROR\r\n");
                return false;
            }
            if (csw.dCSWTag != cbw.dCBWTag) {
                usbdvd_log("TAG ERROR\r\n");
                return false;
            }
            if (csw.bCSWStatus != 0) {
                usbdvd_log("STATUS ERROR\r\n");
                return false;
            }
            if((buf[0] & 0x1F) == 0x05){
                return true;
            }
            return false;
    }
    
    
    
    
    return false;
}