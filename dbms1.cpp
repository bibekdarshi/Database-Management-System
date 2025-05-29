#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

using namespace std;

class Database {
public:
    Database() {}

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
                    throw runtime_error("Error: Expected " + to_string(table.columns.size()) + " values.");
                }
                for (size_t i = 0; i < values.size(); ++i) {
                    if (!isValidType(values[i], table.columns[i].type)) {
                        throw runtime_error("Error: Invalid type for column '" + table.columns[i].name + "'");
                    }
                }
                table.data.push_back(values);
                return;
            }
        }
        throw runtime_error("Error: Table '" + tableName + "' not found.");
    }

    void updateRecord(const string& tableName, int rowIndex, int columnIndex, const string& newValue) {
        for (Table& table : tables) {
            if (table.name == tableName) {
                if (rowIndex >= 0 && rowIndex < table.data.size() && columnIndex >= 0 && columnIndex < table.columns.size()) {
                    if (!isValidType(newValue, table.columns[columnIndex].type)) {
                        throw runtime_error("Error: Invalid type for column update.");
                    }
                    table.data[rowIndex][columnIndex] = newValue;
                    return;
                }
                throw runtime_error("Invalid row or column index: " + tableName);
            }
        }
        throw runtime_error("Table not found: " + tableName);
    }

    void deleteRecord(const string& tableName, int rowIndex) {
        for (Table& table : tables) {
            if (table.name == tableName) {
                if (rowIndex >= 0 && rowIndex < table.data.size()) {
                    table.data.erase(table.data.begin() + rowIndex);
                    return;
                }
                throw runtime_error("Invalid row index: " + tableName);
            }
        }
        throw runtime_error("Table not found: " + tableName);
    }

    void viewRecords(const string& tableName) {
        for (Table& table : tables) {
            if (table.name == tableName) {
                cout << table.name << endl;
                cout << "\t";
                for (const Column& column : table.columns) {
                    cout << column.name << "\t";
                }
                cout << endl;
                for (const vector<string>& row : table.data) {
                    cout << "\t";
                    for (const string& value : row) {
                        cout << value << "\t";
                    }
                    cout << endl;
                }
                return;
            }
        }
        throw runtime_error("Table not found: " + tableName);
    }

    void processQuery(const string& query) {
        stringstream ss(query);
        string command;
        ss >> command;

        try {
            if (command == "CREATE") {
                string temp, tableName;
                ss >> temp >> tableName;
                vector<string> columns, types;
                string col;
                while (ss >> col) {
                    size_t pos = col.find(":");
                    if (pos == string::npos) throw runtime_error("Missing ':' in column definition.");
                    columns.push_back(col.substr(0, pos));
                    types.push_back(col.substr(pos+1));
                }
                createTable(tableName, columns, types);
                cout << "Table '" << tableName << "' created.\n";
            }
            else if (command == "INSERT") {
                string temp, tableName;
                ss >> temp >> tableName >> temp;
                vector<string> values;
                string val;
                while (getline(ss, val, ',')) {
                    values.push_back(val);
                }
                insertRecord(tableName, values);
                cout << "Inserted into " << tableName << endl;
            }
            else if (command == "VIEW") {
                string tableName;
                ss >> tableName;
                viewRecords(tableName);
            }
            else {
                throw runtime_error("Unsupported command: " + command);
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
    cout << "Mini C++ Database System. Type 'exit' to quit.\n";
    while (true) {
        cout << ">> ";
        string query;
        getline(cin, query);
        if (query == "exit") break;
        db.processQuery(query);
    }
    return 0;
}
