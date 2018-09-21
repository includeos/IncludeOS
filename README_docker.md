
# Docker images

These Docker images let you try out building [IncludeOS](https://github.com/hioa-cs/IncludeOS/) unikernels without having to install the development environment locally on your machine.

## Build options
When building the docker image there are a few options available:

|Action| Command |
|--|--|
|Specify build version| ```--build-arg TAG=<git tag/version to build>``` |
|Service to build|```--target=<build/grubify/webserver>```|

### Docker tag structure
The docker tags in use for these images are:
```
<org>/<service>:<IncludeOS_Tag>.<Dockerfile version>
includeos/build:v0.12.0-rc.3.1
```
For every change made to the dockerfile the corresponding tag is incremented.

## Building services
```
$ docker build --build-arg TAG=v0.12.0-rc.3 --target=build -t includeos/build:v0.12.0-rc.3.01 .
$ cd <my-super-cool-service>
$ docker run --rm -v $PWD:/service includeos/build:v0.12.0-rc.3.01
```

## Adding a GRUB bootloader to your service
On macOS, the `boot -g` option to add a GRUB bootloader is not available. Instead, you can use the `includeos/grubify` Docker image.
```
$ docker build --build-arg TAG=v0.12.0-rc.3 --target=grubify -t includeos/grubify:v0.12.0-rc.3.01 .
$ docker run --rm --privileged -v $PWD:/service includeos/grubify:v0.12.0-rc.3.01 build/<image_name>
```

## Running a very basic sanity test of your service image

Don't have a hypervisor installed? No problem? Run your service inside QEMU in a Docker container:

```
$ docker run --rm -v $PWD:/service/build includeos/includeos-qemu:v0.12.0-rc.2.0 <image_name>
```

(If the service is not designed to exit on its own, the container must be stopped with `docker stop`.)


## Building a tiny web server

Do you have some web content that you would like to serve, without having to figure out arcane Apache configuration files? Just go to the folder where your web content is located and build a bootable web server:

```
$ docker build --build-arg TAG=v0.12.0-rc.3 --target=webserver -t includeos/webserver:v0.12.0-rc.3.01 .
docker run --rm -v $PWD:/public includeos/webserver:v0.12.0-rc.3.01
```

## FAQ
### Specify user field to prevent permissions errors
When using bind mounts in docker there are potential errors with user permissions of the files that are mounted. This has been seen on linux systems where a non default uid/gid was used. To work around this we added [Fixuid](https://github.com/boxboat/fixuid) to the build and grubify images. This ensures that the build/grubify container has the correct user permissions to add/modify the required mounted files. If user is not specified then the docker containers use the root user. To specify user add your uid/gid to the docker options:
```
--user $(id -u):$(id -g)
```
