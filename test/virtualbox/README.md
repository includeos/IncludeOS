# Include OS test service

Each subfolder contains a special "seed" (i.e. something that started as a copy of <repo>/seed) which is supposed to build into an authorative test image for IncludeOS. The goal is that whenever you build the test image, it will tell you everything that's rigth and everything that's wrong with the current IncludeOS build.

## Image names
The images created here will have a hashed appendix. That's the short version of the latest git commit hash, given by `git rev-parse --short HEAD`. This way we can differentiate between generations. 

**Note:** We want each image built from one commit to be identical. So if you try to build an image while there are uncommitted changes in the repo, the makefile (using `./img_name.sh`) will call your image "DIRTY".

