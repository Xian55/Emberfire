#pragma once
//
// SqlConnector/QueryResult.h  (reconstructed parent-level support header)
//
// In-memory result set for a SQL query. Reconstructed from client call sites:
//
//   for (auto& fields : result->fetchData())   // fetchData() -> vector<vector<Field>>&
//       fields[0].getString();                 //   row column access by index
//   result->fetchData()[0][0].getInt32();
//   subItr.size();                             //   row is a vector<Field>
//
//   Field column getters used in the client:
//       getString()  -> const std::string&
//       getInt32()   -> int
//       getUInt32()  -> unsigned int
//       getFloat()   -> float
//       getBool()    -> bool
//       getInt64()   -> long long   (provided for completeness / by-name parity)
//
//   QueryResult::data(key, columnName)  -> std::string
//       Looks up the row whose FIRST column == 'key' and returns the value of
//       column 'columnName' (used everywhere as sContentMgr->db("table").data(id,"col")).
//
//   QueryResult::fetchIndexData()       -> std::map<std::string, std::map<std::string, Field>>&
//       Row index keyed by the FIRST column's value; each value is a
//       column-name -> Field map. Used by the *Editor.cpp / MapEditor.cpp:
//           for (auto& itr : db("t").fetchIndexData()) {
//               itr.first;                       // entry-id string
//               auto n = itr.second.find("name");// column lookup
//               n->second.getString();           // Field -> string
//           }
//
//   QueryResult::indexData(key, field, value)  -> void
//       Writes/overwrites a single field in the index (used by the editor save
//       path: editableDb(tbl).indexData(to_string(key), fieldName, str)).
//
//   QueryResult::empty()                -> bool
//
// Compilable standalone under MSVC x86, C++17. Pure container -- no DB backend here.
//
#include <string>
#include <vector>
#include <map>
#include <cstdlib>

// One column value of one row. Values are kept as text (SQLite-style) and
// converted on demand by the typed getters.
class Field
{
public:
    Field() = default;
    explicit Field(const std::string& value) : m_value(value) {}
    explicit Field(std::string&& value) : m_value(std::move(value)) {}

    const std::string& getString() const { return m_value; }

    int           getInt32()  const { return std::atoi(m_value.c_str()); }
    unsigned int  getUInt32() const { return static_cast<unsigned int>(std::strtoul(m_value.c_str(), nullptr, 10)); }
    long long     getInt64()  const { return std::strtoll(m_value.c_str(), nullptr, 10); }
    unsigned long long getUInt64() const { return std::strtoull(m_value.c_str(), nullptr, 10); }
    float         getFloat()  const { return static_cast<float>(std::atof(m_value.c_str())); }
    double        getDouble() const { return std::atof(m_value.c_str()); }
    bool          getBool()   const { return getInt32() != 0; }

    bool isNull() const { return m_isNull; }
    void setNull(bool isNull) { m_isNull = isNull; }

    void setValue(const std::string& value) { m_value = value; m_isNull = false; }

private:
    std::string m_value;
    bool m_isNull{ false };
};

class QueryResult
{
public:
    using Row = std::vector<Field>;

    // Index representation used by the editors: entry-id string -> (column name -> Field).
    // std::map (ordered) so the editors' natural iteration is deterministic and so
    // .find()/.end() behave as the call sites expect.
    using IndexRow   = std::map<std::string, Field>;
    using IndexData  = std::map<std::string, IndexRow>;

    QueryResult() = default;

    // ---- column-name metadata ----
    void setColumnNames(const std::vector<std::string>& names) { m_columnNames = names; }
    const std::vector<std::string>& columnNames() const { return m_columnNames; }

    int columnIndex(const std::string& name) const
    {
        for (size_t i = 0; i < m_columnNames.size(); ++i)
            if (m_columnNames[i] == name)
                return static_cast<int>(i);
        return -1;
    }

    // ---- row storage ----
    std::vector<Row>& fetchData() { return m_rows; }
    const std::vector<Row>& fetchData() const { return m_rows; }

    void addRow(Row&& row) { m_rows.push_back(std::move(row)); }
    void addRow(const Row& row) { m_rows.push_back(row); }

    bool empty() const { return m_rows.empty(); }
    size_t size() const { return m_rows.size(); }

    // ---- index access (entry-id string -> column-name -> Field) ----
    // Lazily built from the row data + column names on first access. Returns a
    // mutable reference: editors iterate it (fetchIndexData()) and the index is
    // also the backing store for data()/indexData().
    IndexData& fetchIndexData()
    {
        buildIndex();
        return m_index;
    }

    const IndexData& fetchIndexData() const
    {
        const_cast<QueryResult*>(this)->buildIndex();
        return m_index;
    }

    // Write/overwrite a single field in the index (editor save path).
    void indexData(const std::string& key, const std::string& field, const std::string& value)
    {
        buildIndex();
        m_index[key].emplace(field, Field(std::string()));
        m_index[key][field].setValue(value);
    }

    // ---- keyed lookup: find the row whose first column == 'key', return 'columnName' ----
    std::string data(const std::string& key, const std::string& columnName) const
    {
        const_cast<QueryResult*>(this)->buildIndex();
        auto rowItr = m_index.find(key);
        if (rowItr == m_index.end())
            return std::string();
        auto colItr = rowItr->second.find(columnName);
        if (colItr == rowItr->second.end())
            return std::string();
        return colItr->second.getString();
    }

    // Integer-keyed convenience overload (callers pass entry ids of various int types).
    std::string data(long long key, const std::string& columnName) const
    {
        return data(std::to_string(key), columnName);
    }

private:
    // Build m_index from the stored rows once. The FIRST column value is the key,
    // and every column (by name) becomes a Field in that key's inner map.
    void buildIndex()
    {
        if (m_indexBuilt)
            return;
        m_indexBuilt = true;

        for (const Row& row : m_rows)
        {
            if (row.empty())
                continue;
            const std::string key = row[0].getString();
            IndexRow& dest = m_index[key];
            const size_t n = row.size();
            for (size_t c = 0; c < n; ++c)
            {
                const std::string& colName =
                    (c < m_columnNames.size()) ? m_columnNames[c] : std::string();
                if (colName.empty())
                    continue;
                dest[colName] = row[c];
            }
        }
    }

    std::vector<std::string> m_columnNames;
    std::vector<Row>         m_rows;
    IndexData                m_index;
    bool                     m_indexBuilt{ false };
};
