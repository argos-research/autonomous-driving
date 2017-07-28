How to run SAVM

git clone -b sub_and_parking https://gitlab.lrz.de/lil4/speedDreams.git

cd speedDreams/genode/repos

ln -s ../../tools/genode-world world

cd speedDreams/

make jenkins_build_dir

make toolchain

make ports

make vde

make jenkins_run
