#!/bin/sh

set -e;

echo "Formatting the codebase...";

find . -type f -name '*.hpp' -exec clang-format -style=file -i {} \;
find . -type f -name '*.cpp' -exec clang-format -style=file -i {} \;

echo "->   Completed successfully!";

exit 0;
