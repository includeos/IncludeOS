echo "Installing build tools we can't live without"
sudo apt install -y emacs cmake python3-pip

echo "Fetching llvm install script"
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18


echo "Installing conan"
pip install conan

echo "NOTE: You might want to export the conan binary path to PATH"
