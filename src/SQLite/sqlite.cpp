#include <SQLite/sqlite.hpp>

#include <stdio.h>
#include <SQLite/sqlite3.h>

namespace database
{
	SQLite::SQLite()
	{
		this->status = sqlite3_open(":memory:", (sqlite3**) &this->database);
		
		printf("Database status: %d\n", this->status);
	}
	SQLite::~SQLite()
	{
		sqlite3_close((sqlite3*) this->database);
	}
}
