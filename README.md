# libusbdvd
A library supporting usb-dvd drives on switch HOS

libusbdvd is a c++ library for HorizonOS supporting CD/DVD Drives.

Current support:
 - CD Audio
 - ISO9660 with Joliet and RockRidge support
 
 
*It support USB MASS STORAGE SUBCLASS 0x02 devices

TODO List:
- UDF Filesystem support
- DECSS Algo
- Blu-ray support 

C Example

    /* Create the lib obj ref */
    usbdvd_obj* dvdtest = usbdvd_create();
    /* ref to the drive ctx with info */
    usbdvd_drive_struct *drivectx = usbdvd_get_drivectx(dvdtest);
    /* destroy and cleanup */
    usbdvd_destroy(dvdtest);
    
C++ Example

    /* Init the USBDVD Class */
    CUSBDVD *dvdtest= new CUSBDVD();
    /* ref to the drive ctx with info */
    usbdvd_drive_struct *drivectx = &dvdtest->usbdvd_drive_ctx;
    /* destroy and cleanup */
    delete dvdtest;




Thanks to
- DarkMatterCore for libusbhsfs https://github.com/DarkMatterCore/libusbhsfs