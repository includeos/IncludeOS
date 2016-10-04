# acorn
Acorn Web Server Appliance, built with [IncludeOS](https://github.com/hioa-cs/IncludeOS).

**Live Demo:** [acorn2.unofficial.includeos.io](http://acorn2.unofficial.includeos.io/) (sporadically unavailable)

## Features

* Easily build RESTful APIs
* Use middleware to browse and serve static content with only a few lines of code
* Optionally show real-time information about your server with an interactive dashboard

Acorn is a simple web server using a collection of libraries and extensions:

* [Mana](https://github.com/includeos/mana) - IncludeOS Web Framework
* [Bucket](https://github.com/includeos/bucket) - Simplified in-memory database
* [Butler](https://github.com/includeos/butler) - Middleware for serving static content
* [Cookie](https://github.com/includeos/cookie) - Cookie support
* [Dashboard](https://github.com/includeos/dashboard) - Back-end support for serving IncludeOS statistics
* [Director](https://github.com/includeos/director) - Middleware for listing static content
* [Json](https://github.com/includeos/json) - JSON support


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
