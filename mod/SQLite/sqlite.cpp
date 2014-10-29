#include <SQLite/sqlite.hpp>

#include <stdio.h>

namespace database
{
	SQLite::SQLite()
	{
		this->status = sqlite3_open(":memory:", &this->database);
		printf("Database status: %d, db: %p\n", this->status, this->database);
	}
	
	SQLite::~SQLite()
	{
		if (this->database)
			sqlite3_close((sqlite3*) this->database);
	}
	
	int SQLite::exec(const std::string& command)
	{
		char* error = nullptr;
		int rc = sqlite3_exec(this->database, command.c_str(), 0, 0, &error);
		
		if (rc)
		{
			printf("SQLite error(%d): %s\n", rc, error);
			sqlite3_free(error);
		}
		return rc;
	}
	
	int SQLite::select(const std::string& query, select_t func)
	{
		sqlite3_stmt* stmt;
		int rc = sqlite3_prepare_v2(this->database,
			query.c_str(), -1, &stmt, nullptr);
		
		if (rc != SQLITE_OK)
		{
			printf("SQLite::select() failed: %s\n", sqlite3_errmsg(this->database));
			return rc;
		}
		
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			SQResult result(stmt);
			int columnCount = sqlite3_column_count(stmt);
			
			for (int index = 0; index < columnCount; index++)
			{
				std::string type = sqlite3_column_decltype(stmt, index);
				
				if (type == "INT")
				{
					result.row.emplace_back(
						SQResult::INTEGER,
						new int(sqlite3_column_int(stmt, index)));
				}
				else if (type == "FLOAT")
				{
					result.row.emplace_back(
						SQResult::FLOAT,
						new double(sqlite3_column_double(stmt, index)));
				}
				else if (type == "TEXT")
				{
					result.row.emplace_back(
						SQResult::TEXT,
						new std::string((char*) sqlite3_column_text(stmt, index)));
				}
				else
				{
					printf("SQLite::select(): Unknown column decltype: %s\n", type.c_str());
					return SQLITE_IOERR_NOMEM;
				}
				
			} // column index
			
			// execute callback to user with result
			func(result);
			
		} // row index
		
		sqlite3_finalize(stmt);
		return SQLITE_OK;
		
	} // select()
	
}
