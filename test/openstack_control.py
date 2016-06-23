#!/usr/bin/env python

"""
Interfaces with openstack to start, stop, create and delete VM's
"""

import os
import sys
import ConfigParser
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

def vm_create(name, 
              image = Config.get('Openstack','image'), 
              key_pair = Config.get('Openstack','key_pair'), 
              flavor = Config.get('Openstack','flavor'),
              network_name = Config.get('Openstack','network_name'),
              network_id = Config.get('Openstack','network_id')):
    """ Creates a VM 

    name = Name of VM
    image = Name of image file to use. (Ubuntu_16.04_LTS, Ubuntu 14.04 LTS)
    key_pair = Name of ssh key pair to inject into VM
    flavor = Resources to dedicate to VM (g1.small/medium/large)
    network_id = Network to connect to
    """
    #print help(nova.servers)
    print key_pair
    print image
    print "image {0}".format(image)
    #nics = [{"net_id": network_id, "v4-fixed-ip": ''}]
    nics = [{"net-id": nova.networks.find(label=network_name).id, "v4-fixed-ip": ''}]
    print nics
    #nova.servers.create(name, 'ec99573c-065f-4ee9-b56c-9fb07e51f322',
    #                    '7671e72b-c575-4bee-9fe0-c62c7d2fcc9b',
    #                    nics=nics)

def vm_delete(name):
    """ Deletes a VM """
    pass

def vm_status(name):
    """ Returns status of VM
        The following is returned as a dictionary:
        id          : Id of server
        name        : Name of server
        status      : Server status, e.g. ACTIVE, BUILDING, DELETED, ERROR
        power_state : Running, Shutdown
        network     : Network info (network, ip as a tuple)
    """
    status_dict = {}

    # Find server
    options = {'name': name }
    server = nova.servers.list(search_opts=options)
    if not server:
        print "No server found with the name: {0}".format(name)
        return
    server = server[0]
    server_info = server.to_dict()

    # ID
    status_dict['id'] = server_info['id']

    # Name
    status_dict['name'] = name

    # Find status
    status_dict['status'] = server_info['status']

    # Power state
    status_dict['power_state'] = server_info['OS-EXT-STS:power_state']

    # Find IP
    networks = server_info['addresses']
    network_id = networks.keys()[0]
    ip = networks[network_id][0]['addr']
    status_dict['network'] = ip

    # Images
    print server_info['image']
    return status_dict


def vm_stop(name):
    """ Stops a VM """
    pass

def vm_start(name):
    """ Starts a VM """
    pass

def main():
    print vm_status('pull_request_1')
    vm_create('test_script')
    return

if __name__ == '__main__':
    main()
