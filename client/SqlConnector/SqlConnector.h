#pragma once
//
// SqlConnector/SqlConnector.h  (reconstructed parent-level support header)
//
// SQLite-backed database wrapper. Reconstructed from client call sites:
//
//   shared_ptr<SqlConnector> db = SqlConnector::create(
//       SqlType::SQLite,
//       sConfig->getString("GameDb", "Name"),
//       sConfig->getString("GameDb", "Path"));
//
//   if (auto result = db->query("SELECT ..."))      // -> shared_ptr<QueryResult>
//       for (auto& fields : result->fetchData()) ...
//
//   db->query(Util::fmtStr("INSERT ..."));          // non-SELECT, result ignored
//   if (!db->error().empty()) ...                   // last error text
//
// query() returns a NON-NULL shared_ptr<QueryResult> ONLY when the statement
// produced at least one row; it returns nullptr for empty results, non-SELECT
// statements, or errors -- this matches every "if (auto result = db->query(...))"
// guard in the client (callers index fetchData()[0][0] only inside that guard).
//
// BACKEND: real sqlite3 C API. Requires the SQLite amalgamation:
//   #include <sqlite3.h>   (add sqlite3.c / sqlite3.h to the build; available at
//   build/deps/sqlite/). The bodies that touch sqlite3 are marked  // [sqlite3].
//
// Compilable standalone under MSVC x86, C++17 (given sqlite3.h on the include path).
//
#include <string>
#include <memory>
#include <vector>

#include "QueryResult.h"

#include <sqlite3.h>   // SQLite amalgamation (build/deps/sqlite/sqlite3.h)

enum class SqlType
{
    SQLite,
    MySQL   // declared for parity with create()'s API; only SQLite is implemented.
};

class SqlConnector
{
public:
    // Factory. 'name' is the database filename, 'path' is the directory holding it.
    static std::shared_ptr<SqlConnector> create(SqlType type,
                                                 const std::string& name,
                                                 const std::string& path)
    {
        auto connector = std::shared_ptr<SqlConnector>(new SqlConnector());
        connector->m_type = type;
        connector->open(name, path);
        return connector;
    }

    ~SqlConnector()
    {
        // [sqlite3]
        if (m_db)
        {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }

    // Execute a statement. Returns a populated QueryResult (non-null) when rows
    // were produced, otherwise nullptr. Updates error() on failure.
    std::shared_ptr<QueryResult> query(const std::string& sql)
    {
        m_lastError.clear();

        // [sqlite3]
        if (!m_db)
        {
            m_lastError = "Database is not open";
            return nullptr;
        }

        sqlite3_stmt* stmt = nullptr;
        const int prep = sqlite3_prepare_v2(m_db, sql.c_str(),
                                            static_cast<int>(sql.size()), &stmt, nullptr);
        if (prep != SQLITE_OK)
        {
            m_lastError = sqlite3_errmsg(m_db);
            if (stmt)
                sqlite3_finalize(stmt);
            return nullptr;
        }

        auto result = std::make_shared<QueryResult>();

        const int columnCount = sqlite3_column_count(stmt);
        std::vector<std::string> columnNames;
        columnNames.reserve(columnCount);
        for (int c = 0; c < columnCount; ++c)
        {
            const char* cn = sqlite3_column_name(stmt, c);
            columnNames.emplace_back(cn ? cn : "");
        }
        result->setColumnNames(columnNames);

        int step = SQLITE_OK;
        bool anyRow = false;
        while ((step = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            anyRow = true;
            QueryResult::Row row;
            row.reserve(columnCount);
            for (int c = 0; c < columnCount; ++c)
            {
                if (sqlite3_column_type(stmt, c) == SQLITE_NULL)
                {
                    Field f{ std::string() };
                    f.setNull(true);
                    row.push_back(std::move(f));
                }
                else
                {
                    const unsigned char* txt = sqlite3_column_text(stmt, c);
                    row.emplace_back(txt ? reinterpret_cast<const char*>(txt) : "");
                }
            }
            result->addRow(std::move(row));
        }

        if (step != SQLITE_DONE)
            m_lastError = sqlite3_errmsg(m_db);

        sqlite3_finalize(stmt);

        if (!anyRow)
            return nullptr;   // no rows -> null (INSERT/UPDATE/empty SELECT)

        return result;
    }

    // Last error message ("" when the previous call succeeded).
    const std::string& error() const { return m_lastError; }

    bool isOpen() const { return m_db != nullptr; }

private:
    SqlConnector() = default;
    SqlConnector(const SqlConnector&) = delete;
    SqlConnector& operator=(const SqlConnector&) = delete;

    void open(const std::string& name, const std::string& path)
    {
        // Config ships Name="game", Path="game.db" where Path is actually the SQLite
        // *file* (not a directory). So the conventional "<path>/<name>" ("game.db/game")
        // is invalid; try it, then the bare path ("game.db") and bare name as fallbacks,
        // accepting the first candidate that opens AND holds a schema (sqlite3_open
        // happily creates empty files, so we must verify real tables exist).
        std::string dir = path;
        if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
            dir += '/';

        const std::string candidates[] = { dir + name, path, name };
        for (const auto& cand : candidates)
        {
            if (cand.empty())
                continue;

            // [sqlite3]
            if (sqlite3_open(cand.c_str(), &m_db) == SQLITE_OK && m_db)
            {
                sqlite3_stmt* st = nullptr;
                if (sqlite3_prepare_v2(m_db, "SELECT 1 FROM sqlite_master LIMIT 1", -1, &st, nullptr) == SQLITE_OK)
                {
                    const int rc = sqlite3_step(st);
                    sqlite3_finalize(st);
                    if (rc == SQLITE_ROW)
                        return;   // opened a database that actually has tables
                }
            }

            if (m_db)
            {
                sqlite3_close(m_db);
                m_db = nullptr;
            }
        }

        m_lastError = "Failed to open database";
    }

    SqlType     m_type{ SqlType::SQLite };
    sqlite3*    m_db{ nullptr };   // [sqlite3]
    std::string m_lastError;
};
