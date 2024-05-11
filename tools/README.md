# IncludeOS tools
The following tools are provided: 
- `vmbuild`: Convert a fully linked IncludeOS elf binary into a bootable disk image, 
  by attaching a bootloader. 
- `elf_syms`: Moves symbols to a custom section, to better support IncludeOS backtrace.
- **TODO:** Consider adding the vmrunner here as well, if we still want it.
  - https://github.com/includeos/vmrunner


## History: 
The tools have been moved out and back in, so the current git history does not tell the full story. 
- 2014: `vmbuilder` created as part of the first IncludeOS release, v0.1.0-proto. 
  - Renamed to `vmbuild` later that year, with v0.3.0-proto.
  - See https://github.com/includeos/IncludeOS/tree/v0.1.0-proto/vmbuilder
- 2016: `elf_syms` tool added by @fwsGonzo
- 2019: Moved to https://github.com/includeos/vmbuild with conan build system.
- 2024: Ported back into IncludeOS/tools with nix build system for easier maintainability.
