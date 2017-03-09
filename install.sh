#!/bin/bash

############################################################
# OPTIONS:
############################################################

# Location of the IncludeOS repo (default: current directory)
export INCLUDEOS_SRC=${INCLUDEOS_SRC-`pwd`}
# Prefered install location (default: /usr/local)
export INCLUDEOS_PREFIX=${INCLUDEOS_PREFIX-/usr/local}
# Enable compilation of tests in cmake (default: OFF)
export INCLUDEOS_ENABLE_TEST=${INCLUDEOS_ENABLE_TEST-OFF}

############################################################
# COMMAND LINE PROPERTIES:
############################################################

# Initialize variables:
install_yes=0
quiet=0
source=0

while getopts "h?yqs" opt; do
    case "$opt" in
    h|\?)
        printf "%s\n" "Options:"\
                "-y Yes: answer yes to install"\
                "-q Quiet: Suppress output from cmake during install"\
                "-s Source: Install from source"
        exit 0
        ;;
    y)  install_yes=1
        ;;
    q)  quiet=1
        ;;
    s)  source=1
        ;;
    esac
done

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

# Install build requirements (compiler, etc) 
if [ "Darwin" = "$SYSTEM" ]; then
    if ! ./etc/install_osx.sh; then
		printf "%s\n" ">>> Sorry <<<"\
			   "Could not install osx dependencies"
		exit 1
	fi
else
	if ! ./etc/install_build_requirements.sh $SYSTEM $RELEASE; then
		printf "%s\n" ">>> Sorry <<<"\
			   "Could not install build requirements."
		exit 1
	fi
fi

############################################################
# INSTALL INCLUDEOS:
############################################################

# Check if script has write permission to PREFIX location
start_dir=$INCLUDEOS_PREFIX
while [ "$start_dir" != "/" ]
do	
	if [ -d $start_dir ]; then	# If dir exists
		if [ ! -w $start_dir ]; then	# If dir is not writable
			printf "\n\n>>> IncludeOS can't be installed with the current options\n"
			printf "    INCLUDEOS_PREFIX is set to %s\n" "$INCLUDEOS_PREFIX"
			printf "    which is not a directory where you have write permissions.\n"
			printf "    Either call install.sh with sudo or set INCLUDEOS_PREFIX\n"
			exit 1
		else
			# Directory exists and is writable, continue install script
			break
		fi
	else
		# If directory is not yet created, check if parent dir is writeable
		start_dir="$(dirname "$start_dir")"
	fi
done

# Print currently set install options
printf "\n\n>>> IncludeOS will be installed with the following options:\n\n"
printf "    %-25s %-25s %s\n"\
	   "Env variable" "Description" "Value"\
	   "------------" "-----------" "-----"\
	   "INCLUDEOS_SRC" "Source dir of IncludeOS" "$INCLUDEOS_SRC"\
	   "INCLUDEOS_PREFIX" "Install location" "$INCLUDEOS_PREFIX"\
	   "INCLUDEOS_ENABLE_TEST" "Enable test compilation" "$INCLUDEOS_ENABLE_TEST"

# Give user option to evaluate install options
if tty -s && [ $install_yes -eq 0 ]; then
	read -p "Is this correct [Y/n]?" answer
	answer=${answer:-"Y"}	# Default value
	case $answer in
		[yY] | [yY][Ee][Ss] )
			true;;
		[nN] | [n|N][O|o] )
			exit 1;;
		*) echo "Invalid input"
		   exit 1;;
	esac
fi

# Trap that cleans the cmake output file in case of exit
function clean {
	if [ -f /tmp/cmake_output.txt ]; then
		rm /tmp/cmake_output.txt
	fi
} 
trap clean EXIT

# if the --all-source parameter was given, build it the hard way
if [ $source -eq 1 ]; then
    printf "\n\n>>> Installing everything from source"
    if ! ./etc/install_all_source.sh; then
        printf  "%s\n" ">>> Sorry <<<"\
				"Could not install from source."
	fi
else
	printf "\n\n>>> Running install_from_bundle.sh (expect up to 3 minutes)\n"
	if [ $quiet -eq 1 ]; then
		if ! ./etc/install_from_bundle.sh &> /tmp/cmake_output.txt; then
			cat /tmp/cmake_output.txt	# Print output because it failed
			printf  "%s\n" ">>> Sorry <<<"\
					"Could not install from bundle."
			exit 1
		fi
	else
		if ! ./etc/install_from_bundle.sh; then
			printf  "%s\n" ">>> Sorry <<<"\
					"Could not install from bundle."
			exit 1
		fi
	fi
fi

# Install network bridge
if [ "Linux" = "$SYSTEM" ]; then
    printf "\n\n>>> Installing network bridge\n"
    if ! ./etc/scripts/create_bridge.sh; then
        printf "%s\n" ">>> Sorry <<<"\
			   "Could not create or configure bridge."
        exit 1
    fi
fi

############################################################
# INSTALL FINISHED:
############################################################

printf "\n\n>>> IncludeOS installation Done!\n" 
printf "    %s\n" "To use IncludeOS set env variables for cmake to know your compiler, e.g.:"\
	   '    export CC="clang-3.8"'\
	   '    export CXX="clang++-3.8"'\
	   ""\
	   "Test your installation with ./test.sh"

# Check if boot command is available
if ! type boot > /dev/null 2>&1; then
	printf "\n    The boot utility is not available, add IncludeOS to your path:\n"
	printf "        export PATH=\$PATH:$INCLUDEOS_PREFIX/bin\n"
fi


exit 0
