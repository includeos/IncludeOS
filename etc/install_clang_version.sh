# Install a specific version of clang
export clang_version=3.6

sudo apt-get install -y clang-$clang_version

# Symlink. 
if ! command clang --version; then
    echo -e "\n\n >>> SYMLINKING CLANG (requires sudo) \n"
    sudo ln -s /usr/bin/clang-$clang_version /usr/bin/clang
fi

if ! command clang++ --version; then
    echo -e "\n\n >>> SYMLINKING CLANG++ (requires sudo) \n"
    sudo ln -s /usr/bin/clang++-$clang_version /usr/bin/clang++
fi

