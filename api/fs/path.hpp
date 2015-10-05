#pragma once
#include <vector>
#include <iostream>

namespace fs
{
	class Path
	{
	public:
		//! constructs Path to the current directory
		Path();
		//! constructs Path to @path
		Path(const std::string& path);
		
    std::size_t size() const
    {
      return stk.size();
    }
    const std::string& operator [] (int i) const
    {
      return stk[i];
    }
    int getState() const { return state; }
    
		Path& operator = (const std::string& p)
		{
			stk.clear();
			this->state = parse(p);
      return *this;
		}
		Path& operator += (const std::string& p)
		{
			this->state = parse(p);
      return *this;
		}
		Path operator + (const std::string& p) const
		{
			Path np  = Path(*this);
      np.state = np.parse(p);
      return np;
		}
		
		bool operator == (const Path& p) const
		{
			return this->toString() == p.toString();
		}
		bool operator == (const std::string& p) const
		{
			return *this == Path(p);
		}
		
		friend std::ostream& operator << (std::ostream& st, const Path& p)
		{
			return st << p.toString();
		}
		
    void up()
    {
      if (!stk.empty())
        stk.pop_back();
    }
		std::string toString() const;
    
	private:
		int parse(const std::string& path);
		int nameAdded(const std::string& name);
		std::string realPath() const;
		
		int state;
    std::vector<std::string> stk;
	};
	
}
