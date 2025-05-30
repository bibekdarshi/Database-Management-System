#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm>

using namespace std;

class Database {
public:
    void createTable(const string& tableName, const vector<string>& columnNames, const vector<string>& columnTypes) {
        if (columnNames.size() != columnTypes.size()) {
            throw runtime_error("Mismatch between column names and types.");
        }
        Table newTable;
        newTable.name = tableName;
        for (int i = 0; i < columnNames.size(); ++i) {
            newTable.columns.push_back({columnNames[i], columnTypes[i]});
        }
        tables.push_back(newTable);
    }

    void insertRecord(const string& tableName, const vector<string>& values) {
        for (Table& table : tables) {
            if (table.name == tableName) {
                if (values.size() != table.columns.size()) {
                    throw runtime_error("Expected " + to_string(table.columns.size()) + " values.");
                }
                for (size_t i = 0; i < values.size(); ++i) {
                    if (!isValidType(values[i], table.columns[i].type)) {
                        throw runtime_error("Invalid type for column '" + table.columns[i].name + "'");
                    }
                }
                table.data.push_back(values);
                return;
            }
        }
        throw runtime_error("Table not found.");
    }

    void viewRecords(const string& tableName, const string& whereCol = "", const string& whereVal = "") {
        for (const Table& table : tables) {
            if (table.name == tableName) {
                int whereIdx = -1;
                if (!whereCol.empty()) {
                    for (int i = 0; i < table.columns.size(); ++i) {
                        if (table.columns[i].name == whereCol) {
                            whereIdx = i;
                            break;
                        }
                    }
                    if (whereIdx == -1) throw runtime_error("Column not found for WHERE condition.");
                }

                cout << table.name << endl << "\t";
                for (const Column& col : table.columns) cout << col.name << "\t";
                cout << endl;

                for (const auto& row : table.data) {
                    if (whereIdx == -1 || row[whereIdx] == whereVal) {
                        cout << "\t";
                        for (const auto& val : row) cout << val << "\t";
                        cout << endl;
                    }
                }
                return;
            }
        }
        throw runtime_error("Table not found.");
    }

    void deleteRecords(const string& tableName, const string& whereCol, const string& whereVal) {
        for (Table& table : tables) {
            if (table.name == tableName) {
                int colIdx = getColumnIndex(table, whereCol);
                if (colIdx == -1) throw runtime_error("Column not found for DELETE.");

                auto& rows = table.data;
                rows.erase(remove_if(rows.begin(), rows.end(),
                    [&](const vector<string>& row) { return row[colIdx] == whereVal; }), rows.end());
                cout << "Records deleted from " << tableName << endl;
                return;
            }
        }
        throw runtime_error("Table not found.");
    }

    void updateRecords(const string& tableName, const string& whereCol, const string& whereVal,
                       const string& updateCol, const string& newVal) {
        for (Table& table : tables) {
            if (table.name == tableName) {
                int whereIdx = getColumnIndex(table, whereCol);
                int updateIdx = getColumnIndex(table, updateCol);
                if (whereIdx == -1 || updateIdx == -1) throw runtime_error("Column not found for UPDATE.");

                if (!isValidType(newVal, table.columns[updateIdx].type)) {
                    throw runtime_error("New value has invalid type.");
                }

                for (auto& row : table.data) {
                    if (row[whereIdx] == whereVal) {
                        row[updateIdx] = newVal;
                    }
                }
                cout << "Records updated in " << tableName << endl;
                return;
            }
        }
        throw runtime_error("Table not found.");
    }

    void createTableFromExisting(const string& newTable, const string& sourceTable) {
        for (const Table& table : tables) {
            if (table.name == sourceTable) {
                Table copy;
                copy.name = newTable;
                copy.columns = table.columns;
                tables.push_back(copy);
                cout << "Table '" << newTable << "' created with structure of '" << sourceTable << "'.\n";
                return;
            }
        }
        throw runtime_error("Source table '" + sourceTable + "' not found.");
    }

    void insertFromTable(const string& destTable, const string& sourceTable) {
        Table *src = nullptr, *dst = nullptr;
        for (Table& table : tables) {
            if (table.name == sourceTable) src = &table;
            if (table.name == destTable) dst = &table;
        }
        if (!src || !dst) throw runtime_error("Source or destination table not found.");
        if (src->columns.size() != dst->columns.size()) {
            throw runtime_error("Column mismatch between tables.");
        }

        for (const auto& row : src->data) {
            dst->data.push_back(row);
        }
        cout << "Records inserted into '" << destTable << "' from '" << sourceTable << "'.\n";
    }

    void processQuery(const string& query) {
        stringstream ss(query);
        string command;
        ss >> command;

        try {
            if (command == "CREATE") {
                string next, tableName;
                ss >> next >> tableName;
                if (next == "TABLE") {
                    string as, select, star, from, srcTable, where, cond;
                    ss >> as >> select >> star >> from >> srcTable >> where >> cond;
                    if (as == "AS" && select == "SELECT" && star == "*" && where == "WHERE" && cond == "1=0;") {
                        createTableFromExisting(tableName, srcTable);
                    } else {
                        // fallback to normal CREATE TABLE col:type col:type...
                        ss.clear(); ss.str(query); // Reset for normal CREATE
                        ss >> command >> next >> tableName;
                        vector<string> columns, types;
                        string col;
                        while (ss >> col) {
                            size_t pos = col.find(":");
                            if (pos == string::npos) throw runtime_error("Missing ':' in column definition.");
                            columns.push_back(col.substr(0, pos));
                            types.push_back(col.substr(pos + 1));
                        }
                        createTable(tableName, columns, types);
                        cout << "Table '" << tableName << "' created.\n";
                    }
                }
            } else if (command == "INSERT") {
                string temp, tableName, select, star, from, sourceTable;
                ss >> temp >> tableName >> select >> star >> from >> sourceTable;
                if (select == "SELECT" && star == "*" && from == "FROM") {
                    insertFromTable(tableName, sourceTable);
                } else {
                    ss.clear(); ss.str(query);
                    ss >> command >> temp >> tableName >> temp;
                    vector<string> values;
                    string val;
                    while (getline(ss, val, ',')) values.push_back(val);
                    insertRecord(tableName, values);
                    cout << "Inserted into " << tableName << endl;
                }
            } else if (command == "VIEW") {
                string tableName, where, col, eq, val;
                ss >> tableName >> where;
                if (where == "WHERE") {
                    ss >> col >> eq >> val;
                    viewRecords(tableName, col, val);
                } else {
                    viewRecords(tableName);
                }
            } else if (command == "DELETE") {
                string tableName, where, col, eq, val;
                ss >> tableName >> where >> col >> eq >> val;
                deleteRecords(tableName, col, val);
            } else if (command == "UPDATE") {
                string tableName, where, wcol, eq, wval, set, ucol, uval;
                ss >> tableName >> where >> wcol >> eq >> wval >> set >> ucol >> uval;
                updateRecords(tableName, wcol, wval, ucol, uval);
            } else {
                throw runtime_error("Unsupported command.");
            }
        } catch (const exception& e) {
            cout << "Query Error: " << e.what() << endl;
        }
    }

private:
    struct Column {
        string name;
        string type;
    };

    struct Table {
        string name;
        vector<Column> columns;
        vector<vector<string>> data;
    };

    vector<Table> tables;

    int getColumnIndex(const Table& table, const string& colName) {
        for (int i = 0; i < table.columns.size(); ++i) {
            if (table.columns[i].name == colName) return i;
        }
        return -1;
    }

    bool isValidType(const string& value, const string& type) {
        stringstream ss(value);
        if (type == "int") {
            int temp; ss >> temp;
            return ss.eof() && !ss.fail();
        } else if (type == "float") {
            float temp; ss >> temp;
            return ss.eof() && !ss.fail();
        } else if (type == "string") {
            return true;
        }
        return false;
    }
};

int main() {
    Database db;
    cout << "Mini C++ Database System with Table Copy Support. Type 'exit' to quit.\n";
    while (true) {
        cout << ">> ";
        string query;
        getline(cin, query);
        if (query == "exit") break;
        db.processQuery(query);
    }
    return 0;
}
