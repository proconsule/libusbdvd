# libusbdvd

libusbdvd is a c++ library for HorizonOS supporting CD/DVD Drives.

Started as P.O.C. for DVD support for [libusbhsfs](https://github.com/DarkMatterCore/libusbhsfs) now is a
DVD/BD drive support library with all major optical fs supported


Current support:
 - USB-DVD USB-BD Drives with USB MASS STORAGE SUBCLASS 0x02
 - USB-DVD USB-BD Drives with USB MASS STORAGE SUBCLASS 0x06 (many Blu-ray drives)
 - File Image mount (.iso) (.cue/.bin)
 - CD Audio
 - ISO9660 with Joliet and basic RockRidge support
 - UDF Support up to 2.60
 - CD/DVD/Blu-ray support
 - Transparent CSS Descramble
 - CSS Key Cache
 - ioctl "fake" file for direct SCSI COMMANDS (needed for AACS Bus Encryption)
 
 
TODO List:
- Exotic disc layout (eg. Mixed Mode CD)

**AACS is not managed by the library, if you want to use AACS protected contents you have to make you own decoder**

For a list of tested device refer to [Compatibility List](https://github.com/proconsule/libusbdvd/blob/master/COMPAT.md)


There is a C++ and C interface (the c program must be linked against -lstdc++)

C Example

    /* Create the lib obj ref */
    usbdvd_obj* dvdtest = usbdvd_init_verbose(1); // 1 = 0x02 and 0x06 subclass 0 = 0x02 subclass only
    /* ref to the drive ctx with info */
    usbdvd_struct *drivectx = usbdvd_get_ctx(dvdtest);
    /* destroy and cleanup */
    usbdvd_destroy(dvdtest);
    
C++ Example

    /* Init the USBDVD Class */
    CUSBDVD *dvdtest= new CUSBDVD();
    /* ref to the drive ctx with info */
    usbdvd_struct *drivectx = &dvdtest->usbdvd_ctx;
    /* destroy and cleanup */
    delete dvdtest;

For image file load you can use

C
 
	usbdvd_obj* dvdtest = usbdvd_initimage("/pathtoimagefile");

C++

	CUSBDVD *dvdtest= new CUSBDVD("/pathtoimagefile");
	


Thanks to

- DarkMatterCore for libusbhsfs https://github.com/DarkMatterCore/libusbhsfs
- libdvdcss library for titlekey routines https://www.videolan.org/developers/libdvdcss.html