# Changelog
<!--
Please categorize a release with the following headings: Added, Changed, Deprecated, Removed, Fixed, Security
Guidelines taken from: https://keepachangelog.com/en/1.0.0/
-->

## v0.15.0

### Added
- Conan build system
  - Major refactoring of how IncludeOS is built
  - Multiple ARCH is managed by Conan profiles and dependencies
  - 3rd party dependencies are now built and managed in Jenkins. All recipes can be found [here](https://github.com/includeos/conan)
    - Updated to libcxx, libcxxabi 7.0.1
    - Updated to GSL 2.0.0
  - Stable and latest binary packages can be found in [bintray](https://bintray.com/includeos/includeos)
  - A repo to install Conan configs for IncludeOS: [conan_config](https://github.com/includeos/conan_config)
  - Improvements to Jenkins integration, automatic uploads of latest/stable packages on master-merge/tags
- Experimental IPv6 (WIP) including SLAAC
  - IPv6/IPv4 dual stack integration
  - TCP/UDP client / server
  - Autoconfiguration with SLAAC
  - Configuration with config.json - see [#2114](https://github.com/includeos/IncludeOS/pull/2114)
- HAL (work in progress)
  - The OS is now backed by a common Machine structure that makes it easier to create new ports
  - A custom C++ allocator is available very early allowing the use of STL before libc is ready

### Changed
- Updates to workflow. All documented in the [README](README.md)
  - No more need for `INCLUDEOS_PREFIX` in env variables
  - Removed ARCH as part of the path to libraries/drivers/plugins/etc
    - Drivers and Plugins can be created outside includeos
- Moved IncludeOS repository from `hioa-cs` to `includeos` organization
- Major breaking changes in the OS API, in particular the OS class is removed, replaced with a smaller os namespace. Much of the code moved to new `kernel::` namespace.
- Relocated plugins/libraries/scripts:
  - [Hello World example](https://github.com/includeos/hello_world)
  - [Demos and examples](https://github.com/includeos/demo-examples)
  - [Mana](https://github.com/includeos/mana)
  - [Uplink](https://github.com/includeos/uplink)
  - [Vmrunner](https://github.com/includeos/vmrunner)
  - [Diskbuilder](https://github.com/includeos/diskbuilder)
  - [Vmbuild](https://github.com/includeos/vmbuild)
  - [MicroLB](https://github.com/includeos/microlb)

### Removed / archived
- Cleanup of unused/outdated scripts
  - `install.sh` is gone as it does no longer work with the Conan workflow
- mender client is [archived](https://github.com/includeos/mender)
