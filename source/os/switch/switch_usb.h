#ifndef SWITCH_USB_H
#define SWITCH_USB_H


#include <stdbool.h>
#include <stdint.h>
#include <switch.h>
#include <mutex>
    
class CSWITCH_USB{
public:
    CSWITCH_USB(bool _unfilter_subclass6 = false,bool verboseinit = false);
    ~CSWITCH_USB();
    
    int usb_bulk_transfer(bool receive,void *buf,uint32_t data_size,uint32_t  *data_transfered,uint32_t timeout);
    Result usb_device_reset();
    Result usbRequestTransfer(UsbHsClientEpSession *usb_ep_session, void *buf, u32 size, u32 *xfer_size,u32 timeout);
    Result usbHsFsRequestGetEndpointStatus(UsbHsClientIfSession *usb_if_session, UsbHsClientEpSession *usb_ep_session, bool *out);
    int usb_clear_halt(UsbHsClientIfSession *usb_if_session,UsbHsClientEpSession *usb_ep_session);
    
    void ResetEndpoints();

    bool device_found = false;  
    uint8_t selsubclass = 0;
    uint8_t subclass_used = 0;
    
private:
    UsbHsInterfaceFilter dvd_filter_sub02;
    Event dvd_usbInterfaceAvailableEvent_sub02;
    UsbHsInterface dvd_usbinterfaces_sub02[8];
    
    UsbHsInterfaceFilter dvd_filter_sub06;
    Event dvd_usbInterfaceAvailableEvent_sub06;
    UsbHsInterface dvd_usbinterfaces_sub06[8];
    
    UsbHsInterface dvd_usbinterfaces[8];
    
    UsbHsClientIfSession inf_session;
    UsbHsClientEpSession endpoint_out;
    UsbHsClientEpSession endpoint_in;
    u8 *xfer_buf; 
    bool unfilter_subclass6 = false;
    std::mutex usb_mutex;
    
    bool check_is_optical();
    
    
};


#endif 