#!/usr/bin/env python

"""
Interfaces with openstack to start, stop, create and delete VM's
"""

import os
import sys
from keystoneauth1.identity import v3
from keystoneauth1 import session
import novaclient.client

# Initiates the authentication used towards OpenStack
auth = v3.Password(auth_url=os.environ['OS_AUTH_URL'],
                   username=os.environ['OS_USERNAME'],
                   password=os.environ['OS_PASSWORD'],
                   project_name=os.environ['OS_PROJECT_NAME'],
                   user_domain_name=os.environ['OS_USER_DOMAIN_NAME'],
                   project_domain_name=os.environ['OS_PROJECT_DOMAIN_NAME'])
sess = session.Session(auth=auth)
nova = novaclient.client.Client(2, session=sess)

def vm_create(name, image, key_pair, flavor):
	""" Creates a VM """
	pass

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

    return status_dict


def vm_stop(name):
	""" Stops a VM """
	pass

def vm_start(name):
	""" Starts a VM """
	pass

def main():
    status = vm_status("jenkins_master")
    print status
    print vm_status('pull_request_1')
    return

if __name__ == '__main__':
	main()
