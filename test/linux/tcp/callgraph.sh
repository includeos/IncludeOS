#! /bin/bash


echo -e "\n>>> Installing dependencies"
pip install gprof2dot
sudo apt install gprof graphviz

echo -e "\n>>> Running tcp test service with profiling"
./run.sh gprof

echo -e "\n>>> Generating graph png"
gprof ./build/linux_tcp | gprof2dot | dot -Tpng -o tcp_callgraph.png
