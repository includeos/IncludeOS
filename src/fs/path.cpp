// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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


#include <fs/path.hpp>

#include <fs/common.hpp>
#include <string>

namespace fs
{
	static const char PATH_SEPARATOR = 0x2F;
	static Path currentDirectory("/");
	
	Path::Path()
		: Path(currentDirectory)
	{
		// uses current directory
	}
	Path::Path(const std::string& path)
	{
		// parse full path
		this->state = parse(path);
		
	} // Path::Path(std::string)
	
	std::string Path::toString() const
	{
		// build path
		//std::stringstream ss;
    std::string ss;
		for (const std::string& P : this->stk)
		{
			ss += PATH_SEPARATOR + P;
		}
		// append path/ to end
		ss += PATH_SEPARATOR;
		return ss;
	}
	
	int Path::parse(const std::string& path)
	{
		if (path.empty())
		{
			// do nothing?
			return 0;
		}
		
		std::string buffer(path.size(), 0);
		char lastChar = 0;
		int  bufi = 0;
		
		for (size_t i = 0; i < path.size(); i++)
		{
			if (path[i] == PATH_SEPARATOR)
			{
				if (lastChar == PATH_SEPARATOR)
				{	// invalid path containing // (more than one forw-slash)
          return -EINVAL;
				}
				if (bufi)
				{
					nameAdded(std::string(buffer, 0, bufi));
					bufi = 0;
				}
				else if (i == 0)
				{
					// if the first character is / separator,
					// the path is relative to root, so clear stack
					stk.clear();
				}
			}
			else
			{
				buffer[bufi] = path[i];
				bufi++;
			}
			lastChar = path[i];
		} // parse path
		if (bufi)
		{
			return nameAdded(std::string(buffer, 0, bufi));
		}
    return 0;
	}
	
	int Path::nameAdded(const std::string& name)
	{
		//std::cout << "Path: " << toString() << " --> " << name << std::endl;
		
		if (name == ".")
		{
			// same directory
		}
		else if (name == "..")
		{
			// if the stack is empty we are at root
			if (stk.empty())
			{
				// trying to go above root is an error
				return -ENOENT;
			}
      stk.pop_back();
		}
		else
		{
			// otherwise treat as directory
			stk.push_back(name);
		}
    return 0;
	}
}
