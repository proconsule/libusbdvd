# libusbdvd

libusbdvd is a c++ library for HorizonOS supporting CD/DVD Drives.

Current support:
 - USB-DVD Drives with USB MASS STORAGE SUBCLASS 0x02
 - File Image mount (.iso)
 - CD Audio
 - ISO9660 with Joliet and basic RockRidge support
 - Basic UDF 1.02 Support
 
 
TODO List:
- Full UDF Filesystem support
- DECSS Algo
- Blu-ray support 

There is a C++ and C interface (the c program must be linked against -lstdc++)

C Example

    /* Create the lib obj ref */
    usbdvd_obj* dvdtest = usbdvd_init();
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

For image file load you can use

C
 
	usbdvd_obj* dvdtest = usbdvd_initimage("/pathtoimagefile");

C++

	CUSBDVD *dvdtest= new CUSBDVD("/pathtoimagefile");
	



Thanks to
- DarkMatterCore for libusbhsfs https://github.com/DarkMatterCore/libusbhsfs