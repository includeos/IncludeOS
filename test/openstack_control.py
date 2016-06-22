#!/usr/bin/env python

"""
Interfaces with openstack to start, stop, create and delete VM's
"""

import os
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
	""" Prints status of a VM """
	server = nova.hosts.get(name)
	print server
	return

def vm_stop(name):
	""" Stops a VM """
	pass

def vm_start(name):
	""" Starts a VM """
	pass

def main():
	vm_status("pull_request_3")
	return

if __name__ == '__main__':
	main()
