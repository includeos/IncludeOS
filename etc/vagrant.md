![IncludeOS Logo](./logo.png)
================================================

## Getting started with IncludeOS development on Vagrant
To get started with IncludeOS development on a Vagrant, you should install Vagrant and virtualbox as instructed.

* [Install Vagrant](https://www.vagrantup.com/docs/installation/)
* [Install VirtualBox](https://www.virtualbox.org/manual/UserManual.html#installation)

After that clone the IncludeOS repo. This will be the basis for bringing up the Vagrant box.

##### Cloning the IncludeOS repository:
```
    $ git clone https://github.com/includeos/IncludeOS
    $ cd IncludeOS/etc
```
##### Now you can create the Vagrant box with IncludeOS  and login to the Vagrant box:
```
	$ vagrant up
	$ vagrant ssh
```
Test the vagrant box installation by creating your first IncludeOS service. Inside Vagrant IncludeOS has been built with profile 'clang-6.0-linux-x86_64', use this profile to create the Hello World service. See [README](/README.md#hello_world).
