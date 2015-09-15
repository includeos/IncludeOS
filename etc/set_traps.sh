set -e # Exit immediately on error (we're trapping the exit signal)
trap 'previous_command=$this_command; this_command=$BASH_COMMAND' DEBUG
trap 'echo -e "\nINSTALL FAILED ON COMMAND: $previous_command\n"' EXIT


# NOTE: script should end with
# trap - EXIT
