This README explains how to to build a custom kernel with the support for TED (Transmission Error Detector) 
and how to test it with the TED application. Different build methods are needed whether you
 want to build the TED kernel for a GNU Linux distribution or Android as for the latter some particular 
steps are required.

#Kernel build
##Linux
###Get kernel sources
Clone the linux kernel repository in your local machine.

	git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linuxrepo

Then move to the version branch you want to build. 
TED patches are currently available for versions 4.1.0 and 3.4.0 but you can always move to a different version branch
and manually adjust the TED patch. Let's assume you chose the 4.1.0 version.

	cd linuxrepo
	git checkout v4.1

In order to be sure your git head is at the desired version, just check the first 3 lines
of the Makefile located in the repository root directory.

###Patch the kernel
If you chose the version 3.4.0 of the kernel you should first apply an official 
 patch needed by TED to work properly. This patch was introduced in later versions and allows
 TED to read some information about its socket at lower levels of the network stack.

From the root directory of the linux kernel repository:

	patch -p1 < path_to_abps_repo/kernel_patches/net-remove-skb_orphan_try.3.4.5.patch

Then apply the TED patch. For the 4.1.0 version only this step is needed to patch the kernel sources.
	
	patch -p1 < path_to_abps_repo/kernel_patches/ted_linux_(your_chosen_version).patch

###Build kernel
Several well documented guides explaining how to build a custom kernel exists in the web. 
Let's pick one as instance:
https://wiki.archlinux.org/index.php/Kernels/Traditional_compilation

As the guide itself explains, for a faster compilation `make localmodconf` 
is recommended but be sure that all the modules you will need later 
are currently loaded.  
At the end just run `make -jN` to start the build 
(where N is the number of parallel compilation processes you want to spawn).

After your custom kernel is built just run with root privileges:

	make modules_install
	make install

or refer to your linux distribution kernel install method.

##Android
- how to enable su
- how to download android sources
- how to patch it, ted patch and sk null fix patch
- use the hammerhead_defconfig_ted
- how to build
- how to make new bootimg with custom kernel
- put files for external usb wifi iface
- how to reboot with custom boot image

TESTAPP:
linux:
- put uapi in the right folder and just make the app
android:
- how to download ndk
- how to copy uapi in ndk for building the application
	cp uapi/errqueue.h uapi/socket.h android-ndk-r11c/platforms/android-21/arch-arm/usr/include/linux/
