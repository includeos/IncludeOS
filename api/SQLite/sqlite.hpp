#ifndef OSABI_SQLITE_SQLITE_HPP
#define OSABI_SQLITE_SQLITE_HPP

#include <functional>
#include <string>
#include <vector>
#include <SQLite/sqlite3.h>

namespace database
{
	struct SQResult
	{
		enum result_t
		{
			INTEGER,
			FLOAT,
			TEXT
		};
		
		SQResult(sqlite3_stmt* statement)
			: stmt(statement) {}
		~SQResult()
		{
			for (auto E : row)
			switch (E.first)
			{
			case INTEGER:
				delete (int*) E.second; break;
			case FLOAT:
				delete (double*) E.second; break;
			case TEXT:
				delete (std::string*) E.second; break;
			}
		}
		
		std::size_t count() const
		{
			return row.size();
		}
		std::pair<result_t, void*> operator[] (int i) const
		{
			return row[i];
		}
		int getInt(int index) const
		{
			return *(int*) row[index].second;
		}
		double getDouble(int index) const
		{
			return *(double*) row[index].second;
		}
		const std::string& getString(int index) const
		{
			return *(std::string*) row[index].second;
		}
		
		std::vector< std::pair<result_t, void*> > row;
		sqlite3_stmt* stmt;
	};
	
	class SQLite
	{
	public:
		SQLite();
		~SQLite();
		
		typedef std::function<void(SQResult&)> select_t;
		
		bool good() const
		{
			return this->status == SQLITE_OK;
		}
		int getStatus() const
		{
			return this->status;
		}
		
		bool isOpen() const
		{
			return database != nullptr;
		}
		sqlite3* getDatabase()
		{
			return database;
		}
		
		int exec(const std::string& query);
		int select(const std::string& query, select_t func);
		
	private:
		sqlite3* database;
		int   status;
	};
}

#endif
