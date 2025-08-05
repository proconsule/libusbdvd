#ifndef SWITCH_USB_H
#define SWITCH_USB_H


#include <stdbool.h>
#include <stdint.h>
#include <switch.h>
#include <mutex>
    
	

class CSWITCH_USB{
public:
	CSWITCH_USB();
	~CSWITCH_USB();
	
	int usb_bulk_transfer(bool receive,void *buf,uint32_t data_size,uint32_t  *data_transfered,uint32_t timeout);
	Result usb_device_reset();
	Result usbRequestTransfer(UsbHsClientEpSession *usb_ep_session, void *buf, u32 size, u32 *xfer_size,u32 timeout);
	Result usbHsFsRequestGetEndpointStatus(UsbHsClientIfSession *usb_if_session, UsbHsClientEpSession *usb_ep_session, bool *out);
	int usb_clear_halt(UsbHsClientIfSession *usb_if_session,UsbHsClientEpSession *usb_ep_session);

	bool device_found = false;	
	
private:
	UsbHsInterfaceFilter dvd_filter;
	Event dvd_usbInterfaceAvailableEvent;
	UsbHsInterface dvd_usbinterfaces[8];
	
	UsbHsClientIfSession inf_session;
	UsbHsClientEpSession endpoint_out;
	UsbHsClientEpSession endpoint_in;
	u8 *xfer_buf; 
	
	std::mutex usb_mutex;
	
};


#endif 