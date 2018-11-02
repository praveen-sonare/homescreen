This project contains:

HomeScreen: AGL Home Screen reference implementation

**AGL repo for source code**

```
$ mkdir WORK
$ cd WORK
$ repo init -b dab -m dab_4.0.0_xml -u https://gerrit.automotivelinux.org/gerrit/AGL/AGL-repo
$ repo sync
$ git clone git clone https://gerrit.automotivelinux.org/gerrit/staging/meta-hmi-framework

```

Then you can get the following recipe.

* `meta-hmi-framework/homescreen-2017`


**Bitbake**

```
$ source meta-agl/scripts/aglsetup.sh -m m3ulcb agl-demo agl-devel agl-appfw-smack agl-hmi-framework
$ bitbake homescreen-2017
```

Instructions for building HomeScreen app
----------------------------------------

The HomeScreen app is part of the
packagegroup-agl-demo-platform
packagegroup.

This also includes the following apps:
- WindowManager
- HomeScreen Binder

And the library
- libhomescreen
- libwindowmanager


To build all the above, follow the instrucions on the AGL
documentation website:
http://docs.automotivelinux.org/docs/getting_started/en/dev/reference/source-code.html#features-supported-by-aglsetup

Please activate the "agl-demo" feature when running the aglsetup script:
http://docs.automotivelinux.org/docs/getting_started/en/dev/reference/source-code.html#features-supported-by-aglsetup


Launch HomeScreen App:

Usage:

```
afm-util start homescreen-2017
```
