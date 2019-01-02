# -*- mode: ruby -*-
# vi: set ft=ruby :

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.define "IncludeOS" do |config|
    config.vm.box = "ubuntu/xenial64"
    config.vm.provider :virtualbox do |vb|
      vb.name = "IncludeOS"
    end

    config.vm.synced_folder ".", "/vagrant", disable: true

    config.vm.synced_folder ".", "/IncludeOS", create: true
    config.vm.provision "shell",
                        inline: "echo cd /IncludeOS >> /home/ubuntu/.bashrc"

    config.vm.synced_folder ".",
                            "/home/vagrant/IncludeOS", create: true

    config.vm.provision "shell", inline: "apt-get update && apt-get install -qq git"
  end
end
