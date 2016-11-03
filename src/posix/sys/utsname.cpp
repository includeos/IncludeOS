// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <sys/utsname.h>
#include <os>

int uname(struct utsname *name) {

	strcpy(name->sysname, "IncludeOS");

	/* sprintf(name->nodename, "Node %d\n", _Objects_Local_node); */
	strcpy(name->nodename, "IncludeOS-node");
	// same as hostname

	strcpy(name->release, OS::version().c_str());

	strcpy(name->version, OS::version().c_str());
	
	/* sprintf(name->machine, "%s/%s", CPU_NAME, CPU_MODEL_NAME); */
	// strcpy(name->machine, OS::machine().c_str());
	strcpy(name->machine, "x86_64");

	return 0;
}