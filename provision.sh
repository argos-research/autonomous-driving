
#######################
#
# This is a provision script
# it will be called once when the vagrant vm is first provisioned
# If you have commands that you want to run always please have a
# look at the bootstrap.sh script
#
# Contributor: Bernhard Blieninger
######################
sudo apt-get update -qq
sudo apt-get install libncurses5-dev texinfo autogen autoconf2.64 g++ libexpat1-dev flex bison gperf cmake libxml2-dev libtool zlib1g-dev libglib2.0-dev make pkg-config gawk subversion expect git libxml2-utils syslinux xsltproc yasm iasl lynx unzip qemu wget alsa-base alsa-utils pulseaudio pulseaudio-utils ubuntu-desktop -qq
cd /vagrant/
sudo rm -rf contrib/
cd /vagrant/
wget 'https://sourceforge.net/projects/genode/files/genode-toolchain/16.05/genode-toolchain-16.05-x86_64.tar.bz2'
sudo tar xPfj genode-toolchain-16.05-x86_64.tar.bz2
cd /vagrant/
sudo tar jxvf libs.tar.bz2
cd /vagrant/
make
cd /vagrant/
sudo chown -R ubuntu /build
sudo echo ubuntu:vagrant | /usr/sbin/chpasswd
sudo reboot
