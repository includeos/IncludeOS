## Template files for new IncludeOS projects

This folder is a base folder you can use for new IncludeOS projects.
It contains three files: 

### `new.sh`
Use this file to initialize the files for your new project. Simply
run: `./new.sh my_project`
This command will modify Makefile and run.sh as well as create a
ready-to-compile file my_project.cpp with "Hello World!" code. 

### `Makefile`
The Makefile will be modified with the new name of the project. When
running the make command, you'll end up with a bootable IncludeOS
image with the same name of your project, like "my_project.img" in the
case above.

### `run.sh`
Use this file to boot your image. It will get the name of the image
automatically from the ./new.sh script.

## Quick start

(Assuming you have installed IncludeOS earlier)

    cp -r IncludeOS/examples/project_template ~/my_project
    cd ~/my_project
    ./new.sh my_project
    make
    ./run.sh
    [ ctrl+a x ] to exit VM
