# -*- mode: ruby -*-
# vi: set ft=ruby :

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.define "IncludeOS" do |config|
    config.vm.box = "ubuntu/trusty64"
    config.vm.box_url = "https://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-amd64-vagrant-disk1.box"
    config.vm.provider :virtualbox do |vb|
      vb.name = "IncludeOS"
    end

    config.vm.synced_folder ".", "/vagrant", disable: true

    config.vm.synced_folder ".", "/IncludeOS", create: true
    config.vm.provision "shell",
                        inline: "echo cd /IncludeOS >> /home/vagrant/.bashrc"

    config.vm.synced_folder "~/IncludeOS_install",
                            "/home/vagrant/IncludeOS_install", create: true

    config.vm.provision "shell", inline: "apt-get update && apt-get install -qq git"
  end
end
