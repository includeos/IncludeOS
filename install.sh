#!/bin/sh

############################################################
# OPTIONS:
############################################################

# Location of the IncludeOS repo (default: current directory)
INCLUDEOS_SRC=${INCLUDEOS_SRC-`pwd`}
# Prefered install location (default: /usr/local)
INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX-/usr/local}
# Enable compilation of tests in cmake (default: OFF)
INCLUDEOS_ENABLE_TEST=${INCLUDEOS_ENABLE_TEST-OFF}

############################################################
# SYSTEM PROPERTIES:
############################################################

SYSTEM=`uname -s`

read_linux_release() {
    LINE=`grep "^ID=" /etc/os-release`
    echo "${LINE##*=}"
}

RELEASE=$([ $SYSTEM = "Darwin" ] && echo `sw_vers -productVersion` || read_linux_release)

[ "$RELEASE" = "neon" ] && RELEASE="ubuntu"

check_os_support() {
    SYSTEM=$1
    RELEASE=$2

    case $SYSTEM in
        "Darwin")
            return 0;
            ;;
        "Linux")
            case $RELEASE in
                "debian"|"ubuntu"|"linuxmint")
                    return 0;
                    ;;
                "fedora")
                    export INCLUDEOS_SRC=`pwd`
                    return 0;
                    ;;
                "arch")
                    return 0;
                    ;;
            esac
    esac
    return 1;
}

# check if system is supported at all
if ! check_os_support $SYSTEM $RELEASE; then
    printf "%s\n" ">>> Sorry <<<"\
		   "Currently only Debian testing/jessie backports, Ubuntu, Fedora, Arch,"\
		   "and OSX are actively supported for *building* IncludeOS."\
		   "On other Linux distros it shouldn't be that hard to get it to work - take"\
		   "a look at ./etc/install_from_bundle.sh"
    exit 1
fi

############################################################
# DEPENDENCIES:
############################################################

# check if sudo is available
if ! command -v sudo > /dev/null 2>&1; then
    printf "%s\n" ">>> Sorry <<<"\
		   "The command sudo was not found."
    exit 1
fi

# now install build requirements (compiler, etc). This was moved into
# a function of its own as it can easen the setup.
if ! ./etc/install_build_requirements.sh $SYSTEM $RELEASE; then
    printf "%s\n" ">>> Sorry <<<"\
		   "Could not install build requirements."
    exit 1
fi

############################################################
# INSTALL INCLUDEOS:
############################################################

# Perform a check of required environment variables
printf "\n\n>>> IncludeOS will be installed with the following options:\n\n"
printf "%-25s %-25s %s\n"\
	   "Env variable" "Description" "Value"\
	   "------------" "-----------" "-----"\
	   "INCLUDEOS_SRC" "Source dir of IncludeOS" "$INCLUDEOS_SRC"\
	   "INCLUDEOS_PREFIX" "Install location" "$INCLUDEOS_PREFIX"\
	   "INCLUDEOS_ENABLE_TEST" "Enable test compilation" "$INCLUDEOS_ENABLE_TEST"

if tty -s; then
	read -p "Is this correct [Y|N]?" answer
	case $answer in
		[yY] | [yY][Ee][Ss] )
			true;;
		[nN] | [n|N][O|o] )
			exit 1;;
		*) echo "Invalid input";;
	esac
fi

# if the --all-source parameter was given, build it the hard way
if [ "$1" = "--all-source" ]; then
    printf "\n\n>>> Installing everything from source"
    ./etc/install_all_source.sh

elif [ "Darwin" = "$SYSTEM" ]; then
    # TODO: move build dependencies to the install build requirements step
    ./etc/install_osx.sh

elif [ "Linux" = "$SYSTEM" ]; then
    printf "\n\n>>> Calling install_from_bundle.sh script"
    if ! ./etc/install_from_bundle.sh; then
        printf  "%s\n" ">>> Sorry <<<"\
				"Could not install from bundle."
        exit 1
    fi

	# Installing network bridge
    printf "\n\n>>> Installing network bridge\n"
    if ! ./etc/scripts/create_bridge.sh; then
        printf "%s\n" ">>> Sorry <<<"\
			   "Could not create or configure bridge."
        exit 1
    fi


fi

printf "\n\n>>> IncludeOS installation Done!\n" 
printf "%s\n" "To use IncludeOS set env variables for cmake to know your compiler, e.g.:"\
	   '    export CC="clang-3.8"'\
	   '    export CXX="clang++-3.8"'\
	   ""\
	   "Test your installation with ./test.sh"

# Check if boot command is available
if ! type boot > /dev/null 2>&1; then
	printf "\nThe boot utility is not available, add IncludeOS to your path:\n"
	printf "    export PATH=\$PATH:$INCLUDEOS_PREFIX/bin"
fi


exit 0
