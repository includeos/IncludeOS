# LiveUpdate

# Put Liveupdate in conan editable mode

**NOTE:  Make sure you are in the IncludeOS src directory**
1. Remove the version variable from the `conanfile.py`.
```
awk '!/version = conan_tools/' lib/LiveUpdate/conanfile.py > temp && mv temp lib/LiveUpdate/conanfile.py
```
1. Add editable package with correct layout
```
conan editable add lib/LiveUpdate liveupdate/9.9.9@includeos/latest --layout lib/LiveUpdate/layout.txt
```
1. Run Conan install
```
conan install -if lib/LiveUpdate/build lib/LiveUpdate
```
1. Run Conan build
```
conan build -sf . -bf lib/LiveUpdate/build lib/LiveUpdate
```
