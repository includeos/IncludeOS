echo "Installing build tools we can't live without"
sudo apt install -y emacs cmake python3-pip clang-18 libc++-18-dev libc++abi-18-dev

echo "Installing conan"
pip install conan

echo "NOTE: You might want to export the conan binary path to PATH"
