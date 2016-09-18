# acorn
Acorn Web Server Appliance, built with [IncludeOS](https://github.com/hioa-cs/IncludeOS).

## Requirements
* [IncludeOS](https://github.com/hioa-cs/IncludeOS) installed (together with its dependencies)
* git

## Setup
To setup the service:
```
$ ./setup.sh
```
It will do the following:
* Pull the required git submodules
* Create a 2 MB FAT16 disk called `memdisk.fat` - which will be included into service
* Mount it on `mnt/` and copy the content from `disk1/` onto the disk

## Take it for a spin
To build and run the service:
```
$ ./run.sh
```
It will do the following:
* Mount and update `memdisk.fat` with the latest content from `disk1/`
* `make` the service
* Run the service with `qemu`
