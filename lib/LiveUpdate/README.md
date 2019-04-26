# LiveUpdate

# Put Liveupdate in conan editable mode

1. Make sure you are in the liveupdate directory for the commands below to work.
```
cd <IncludeOS src>/lib/LiveUpdate
```
2. Remove the version variable from the `conanfile.py`.
```
awk '!/version = conan_tools/' conanfile.py > temp && mv temp conanfile.py
```
3. Create a `build` directory in the LiveUpdate directory.
```
mkdir build
```
4. Add editable package with correct layout
```
conan editable add . liveupdate/9.9.9@includeos/latest --layout layout.txt
```
5. Run Conan install
```
conan install -if build .
```
6. Run Conan build
```
conan build -sf ../../ -bf build .
```
