#!/usr/bin/env python

"""
Interfaces with openstack to start, stop, create and delete VM's
"""

import os
import ConfigParser
import time
from keystoneauth1.identity import v3
from keystoneauth1 import session
import novaclient.client

# Initiates the ConfigParser
Config = ConfigParser.ConfigParser()
Config.read('openstack.conf')

# Initiates the authentication used towards OpenStack
auth = v3.Password(auth_url=os.environ['OS_AUTH_URL'],
                   username=os.environ['OS_USERNAME'],
                   password=os.environ['OS_PASSWORD'],
                   project_name=os.environ['OS_PROJECT_NAME'],
                   user_domain_name=os.environ['OS_USER_DOMAIN_NAME'],
                   project_domain_name=os.environ['OS_PROJECT_DOMAIN_NAME'])
sess = session.Session(auth=auth)
nova = novaclient.client.Client(2, session=sess)
# glance = glclient.Client(2, session=sess)


def vm_create(name,
              image=Config.get('Openstack', 'image'),
              key_pair=Config.get('Openstack', 'key_pair'),
              flavor=Config.get('Openstack', 'flavor'),
              network_name=Config.get('Openstack', 'network_name'),
              network_id=Config.get('Openstack', 'network_id')):
    """ Creates a VM

    name = Name of VM
    image = Name of image file to use. (Ubuntu_16.04_LTS, Ubuntu 14.04 LTS)
    key_pair = Name of ssh key pair to inject into VM
    flavor = Resources to dedicate to VM (g1.small/medium/large)
    network_id = Network to connect to
    """

    nics = [{"net-id": nova.networks.find(label=network_name).id,
             "v4-fixed-ip": ''}]
    nova.servers.create(name,
                        image=image,
                        flavor=flavor,
                        nics=nics,
                        key_name=key_pair)


def vm_delete(name):
    """ Deletes a VM """

    print "vm_delete: Will delete VM: {0}".format(name)
    vm_info = vm_status(name)
    try:
        vm_status(name)['server'].delete()
    except TypeError:
        print "vm_delete: No VM to delete: {0}".format(name)
    return


def vm_status(name):
    """ Returns status of VM
        The following is returned as a dictionary:
        server      : Openstack server object
        id          : Id of server
        name        : Name of server
        status      : Server status, e.g. ACTIVE, BUILDING, DELETED, ERROR
        power_state : Will return 1 if running
        network     : Network info (network, ip as a tuple)
    """
    status_dict = {}

    # Find server
    options = {'name': name}
    server = nova.servers.list(search_opts=options)
    if not server:
        print "No server found with the name: {0}".format(name)
        return
    server = server[0]
    status_dict['server'] = server
    server_info = server.to_dict()

    # ID
    status_dict['id'] = server_info['id']

    # Name
    status_dict['name'] = name

    # Find status
    status_dict['status'] = server_info['status']

    # Power state
    # If running will return 1
    status_dict['power_state'] = server_info['OS-EXT-STS:power_state']

    # Find IP
    networks = server_info['addresses']
    network_id = networks.keys()[0]
    ip = networks[network_id][0]['addr']
    status_dict['network'] = ip

    return status_dict


def vm_stop(name):
    """ Stops a VM, will wait until it has finished stopping before returning
    """

    vm_info = vm_status(name)
    if vm_info['power_state'] == 1:
        print "vm_stop: Will stop VM: {0}".format(name)
        vm_info['server'].stop()
        while vm_status(name)['power_state'] == 1:
            time.sleep(1)
    else:
        print "vm_stop: {0} is not running".format(name)
    return


def vm_start(name):
    """ Starts a VM, will wait until it has finished booting before returning
    """
    vm_info = vm_status(name)
    if vm_info['power_state'] != 1:
        print "vm_start: Will start VM: {0}".format(name)
        vm_info['server'].start()
        while vm_status(name)['power_state'] != 1:
            time.sleep(1)
    else:
        print "vm_start: VM is already running: {0}".format(name)
    return


def main():
    name = 'test_script'
    vm_delete(name)
    return

if __name__ == '__main__':
    main()
