#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlobj.h>
#include <sqlite3.h>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include "embedded_assets.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "sqlite3.lib")

using namespace std;

// Globals
sqlite3* db = nullptr;
string g_storageDir = "";
string g_dbPath = "";
int g_port = 4829;

// Helper: Escape string for JSON
string jsonEscape(const string& s) {
    ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << hex << setw(4) << setfill('0') << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

// Helper: Simple JSON string extractor
string jsonGetString(const string& json, const string& key, const string& def = "") {
    string target = "\"" + key + "\"";
    size_t pos = json.find(target);
    if (pos == string::npos) return def;
    pos = json.find(':', pos);
    if (pos == string::npos) return def;
    
    // Skip whitespace
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }
    if (pos >= json.size()) return def;

    if (json[pos] == '"') {
        pos++;
        string val = "";
        while (pos < json.size()) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                val += json[pos + 1];
                pos += 2;
            } else if (json[pos] == '"') {
                break;
            } else {
                val += json[pos++];
            }
        }
        return val;
    } else {
        // Unquoted value (number or boolean)
        size_t end = json.find_first_of(",}\n\r\t ", pos);
        if (end == string::npos) end = json.size();
        return json.substr(pos, end - pos);
    }
}

// Helper: Boolean JSON extractor
bool jsonGetBool(const string& json, const string& key, bool def = true) {
    string target = "\"" + key + "\"";
    size_t pos = json.find(target);
    if (pos == string::npos) return def;
    pos = json.find(':', pos);
    if (pos == string::npos) return def;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }
    if (pos < json.size()) {
        if (json.compare(pos, 4, "true") == 0 || json.compare(pos, 4, "TRUE") == 0) return true;
        if (json.compare(pos, 5, "false") == 0 || json.compare(pos, 5, "FALSE") == 0) return false;
    }
    return def;
}

// Generate simple unique ID
string generateId() {
    static int counter = 0;
    counter++;
    ostringstream oss;
    oss << "app_" << (unsigned long)time(nullptr) << "_" << GetTickCount() << "_" << counter;
    return oss.str();
}

// Helper: Create directory and parent directories recursively
bool createDirectoriesRecursive(const string& path) {
    if (path.empty()) return false;
    char tmp[MAX_PATH];
    strncpy(tmp, path.c_str(), sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char orig = *p;
            *p = '\0';
            if (!(strlen(tmp) == 2 && tmp[1] == ':')) {
                CreateDirectoryA(tmp, NULL);
            }
            *p = orig;
        }
    }
    return CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// Helper: Get standard Windows AppData base storage directory
string getDefaultAppDataDir() {
    char* appdata = getenv("APPDATA");
    if (!appdata) {
        char path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path) == S_OK) {
            return string(path) + "\\JobApplicationTracker";
        }
        return ".\\JobApplicationTracker";
    }
    return string(appdata) + "\\JobApplicationTracker";
}

// Helper: Get path to storage config file
string getConfigFile() {
    return getDefaultAppDataDir() + "\\config.json";
}

// Helper: Load configured database directory from config.json
string loadConfiguredDbDir() {
    string configFile = getConfigFile();
    ifstream in(configFile.c_str());
    if (!in.is_open()) return "";
    string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    return jsonGetString(content, "db_dir", "");
}

// Helper: Save configured database directory to config.json
bool saveConfiguredDbDir(const string& dir) {
    string defDir = getDefaultAppDataDir();
    createDirectoriesRecursive(defDir);
    string configFile = getConfigFile();
    ofstream out(configFile.c_str());
    if (!out.is_open()) return false;
    out << "{\n  \"db_dir\": \"" << jsonEscape(dir) << "\"\n}\n";
    return true;
}

// Ensure database schema and tables exist
bool ensureDatabaseSchema() {
    if (!db) return false;
    const char* sqlCreate = 
        "CREATE TABLE IF NOT EXISTS applications ("
        "  id TEXT PRIMARY KEY,"
        "  company TEXT NOT NULL,"
        "  role TEXT NOT NULL,"
        "  status TEXT NOT NULL DEFAULT 'Applied',"
        "  work_mode TEXT DEFAULT 'Remote',"
        "  job_type TEXT DEFAULT 'Full-time',"
        "  location TEXT DEFAULT '',"
        "  job_url TEXT DEFAULT '',"
        "  salary_min REAL DEFAULT 0,"
        "  salary_max REAL DEFAULT 0,"
        "  currency TEXT DEFAULT '$',"
        "  salary_period TEXT DEFAULT 'Yearly',"
        "  date_applied TEXT DEFAULT '',"
        "  next_interview TEXT DEFAULT '',"
        "  follow_up_date TEXT DEFAULT '',"
        "  contact_name TEXT DEFAULT '',"
        "  contact_email TEXT DEFAULT '',"
        "  contact_phone TEXT DEFAULT '',"
        "  notes TEXT DEFAULT '',"
        "  tags TEXT DEFAULT '',"
        "  created_at TEXT DEFAULT (datetime('now', 'localtime')),"
        "  updated_at TEXT DEFAULT (datetime('now', 'localtime'))"
        ");"
        "CREATE TABLE IF NOT EXISTS activity_log ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  action_type TEXT NOT NULL,"
        "  message TEXT NOT NULL,"
        "  application_id TEXT,"
        "  created_at TEXT DEFAULT (datetime('now', 'localtime'))"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sqlCreate, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "SQLite table init error: " << (errMsg ? errMsg : "unknown") << endl;
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// Initialize SQLite database (using saved custom directory or Windows internal AppData default)
bool initDatabase() {
    string defaultDir = getDefaultAppDataDir();
    createDirectoriesRecursive(defaultDir);

    string configuredDir = loadConfiguredDbDir();
    if (!configuredDir.empty()) {
        g_storageDir = configuredDir;
    } else {
        g_storageDir = defaultDir;
    }

    createDirectoriesRecursive(g_storageDir);
    string attachDir = g_storageDir + "\\attachments";
    CreateDirectoryA(attachDir.c_str(), NULL);
    string backupDir = g_storageDir + "\\backups";
    CreateDirectoryA(backupDir.c_str(), NULL);

    g_dbPath = g_storageDir + "\\jobtracker.db";

    if (sqlite3_open(g_dbPath.c_str(), &db) != SQLITE_OK) {
        cerr << "Failed to open SQLite database at: " << g_dbPath << ". Falling back to AppData default." << endl;
        g_storageDir = defaultDir;
        g_dbPath = g_storageDir + "\\jobtracker.db";
        createDirectoriesRecursive(g_storageDir);
        if (sqlite3_open(g_dbPath.c_str(), &db) != SQLITE_OK) {
            cerr << "Failed to open default SQLite database: " << sqlite3_errmsg(db) << endl;
            return false;
        }
    }

    return ensureDatabaseSchema();
}

// Dynamically change and save database storage location to any folder
bool changeDatabaseLocation(string newDir, bool moveExisting, string& err) {
    while (!newDir.empty() && (newDir.back() == ' ' || newDir.back() == '\t' || newDir.back() == '\r' || newDir.back() == '\n')) {
        newDir.pop_back();
    }
    while (newDir.size() > 3 && (newDir.back() == '\\' || newDir.back() == '/')) {
        newDir.pop_back();
    }

    if (newDir.empty()) {
        newDir = getDefaultAppDataDir();
    }

    // Expand environment variables
    char expanded[MAX_PATH];
    if (ExpandEnvironmentStringsA(newDir.c_str(), expanded, MAX_PATH) > 0) {
        newDir = string(expanded);
    }

    for (char& c : newDir) {
        if (c == '/') c = '\\';
    }

    if (!createDirectoriesRecursive(newDir)) {
        err = "Failed to create directory: " + newDir;
        return false;
    }

    string newAttachDir = newDir + "\\attachments";
    CreateDirectoryA(newAttachDir.c_str(), NULL);
    string newBackupDir = newDir + "\\backups";
    CreateDirectoryA(newBackupDir.c_str(), NULL);

    string newDbPath = newDir + "\\jobtracker.db";

    if (_stricmp(g_dbPath.c_str(), newDbPath.c_str()) == 0) {
        saveConfiguredDbDir(newDir == getDefaultAppDataDir() ? "" : newDir);
        return true;
    }

    if (moveExisting && !g_dbPath.empty()) {
        if (db) {
            sqlite3_wal_checkpoint_v2(db, NULL, SQLITE_CHECKPOINT_FULL, NULL, NULL);
            sqlite3_close(db);
            db = nullptr;
        }

        if (GetFileAttributesA(g_dbPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            if (!CopyFileA(g_dbPath.c_str(), newDbPath.c_str(), FALSE)) {
                sqlite3_open(g_dbPath.c_str(), &db);
                err = "Failed to copy database file to " + newDbPath;
                return false;
            }
        }
    } else {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    if (sqlite3_open(newDbPath.c_str(), &db) != SQLITE_OK) {
        err = "Failed to open SQLite database at: " + newDbPath;
        sqlite3_open(g_dbPath.c_str(), &db);
        return false;
    }

    ensureDatabaseSchema();

    g_storageDir = newDir;
    g_dbPath = newDbPath;

    saveConfiguredDbDir(newDir == getDefaultAppDataDir() ? "" : newDir);
    return true;
}

// Native Windows folder picker dialog
string chooseFolderDialog() {
    bool coInited = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
    char path[MAX_PATH] = {0};
    BROWSEINFOA bi = {0};
    bi.lpszTitle = "Select New Database Storage Folder for APPLY TRACK";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) {
        SHGetPathFromIDListA(pidl, path);
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
    }
    if (coInited) {
        CoUninitialize();
    }
    return string(path);
}

// Log activity
void logActivity(const string& type, const string& msg, const string& appId = "") {
    const char* sql = "INSERT INTO activity_log (action_type, message, application_id, created_at) VALUES (?, ?, ?, datetime('now', 'localtime'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, msg.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, appId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

// Fetch all applications as JSON
string getApplicationsJson() {
    const char* sql = "SELECT id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags, created_at, updated_at FROM applications ORDER BY date_applied DESC, created_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "[]";
    }

    ostringstream out;
    out << "[";
    bool first = true;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) out << ",";
        first = false;

        auto getText = [&](int col) -> string {
            const unsigned char* val = sqlite3_column_text(stmt, col);
            return val ? (const char*)val : "";
        };

        out << "{";
        out << "\"id\":\"" << jsonEscape(getText(0)) << "\",";
        out << "\"company\":\"" << jsonEscape(getText(1)) << "\",";
        out << "\"role\":\"" << jsonEscape(getText(2)) << "\",";
        out << "\"status\":\"" << jsonEscape(getText(3)) << "\",";
        out << "\"work_mode\":\"" << jsonEscape(getText(4)) << "\",";
        out << "\"job_type\":\"" << jsonEscape(getText(5)) << "\",";
        out << "\"location\":\"" << jsonEscape(getText(6)) << "\",";
        out << "\"job_url\":\"" << jsonEscape(getText(7)) << "\",";
        out << "\"salary_min\":" << sqlite3_column_double(stmt, 8) << ",";
        out << "\"salary_max\":" << sqlite3_column_double(stmt, 9) << ",";
        out << "\"currency\":\"" << jsonEscape(getText(10)) << "\",";
        out << "\"salary_period\":\"" << jsonEscape(getText(11)) << "\",";
        out << "\"date_applied\":\"" << jsonEscape(getText(12)) << "\",";
        out << "\"next_interview\":\"" << jsonEscape(getText(13)) << "\",";
        out << "\"follow_up_date\":\"" << jsonEscape(getText(14)) << "\",";
        out << "\"contact_name\":\"" << jsonEscape(getText(15)) << "\",";
        out << "\"contact_email\":\"" << jsonEscape(getText(16)) << "\",";
        out << "\"contact_phone\":\"" << jsonEscape(getText(17)) << "\",";
        out << "\"notes\":\"" << jsonEscape(getText(18)) << "\",";
        out << "\"tags\":\"" << jsonEscape(getText(19)) << "\",";
        out << "\"created_at\":\"" << jsonEscape(getText(20)) << "\",";
        out << "\"updated_at\":\"" << jsonEscape(getText(21)) << "\"";
        out << "}";
    }

    sqlite3_finalize(stmt);
    out << "]";
    return out.str();
}

// Fetch activities JSON
string getActivityJson() {
    const char* sql = "SELECT id, action_type, message, application_id, created_at FROM activity_log ORDER BY id DESC LIMIT 25;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return "[]";

    ostringstream out;
    out << "[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) out << ",";
        first = false;
        out << "{";
        out << "\"id\":" << sqlite3_column_int(stmt, 0) << ",";
        out << "\"action_type\":\"" << jsonEscape((const char*)sqlite3_column_text(stmt, 1)) << "\",";
        out << "\"message\":\"" << jsonEscape((const char*)sqlite3_column_text(stmt, 2)) << "\",";
        const char* appVal = (const char*)sqlite3_column_text(stmt, 3);
        out << "\"application_id\":\"" << jsonEscape(appVal ? appVal : "") << "\",";
        out << "\"created_at\":\"" << jsonEscape((const char*)sqlite3_column_text(stmt, 4)) << "\"";
        out << "}";
    }
    sqlite3_finalize(stmt);
    out << "]";
    return out.str();
}

// Save or Update application
string saveApplicationFromPayload(const string& payload) {
    string id = jsonGetString(payload, "id");
    string company = jsonGetString(payload, "company");
    string role = jsonGetString(payload, "role");
    string status = jsonGetString(payload, "status", "Applied");
    string work_mode = jsonGetString(payload, "work_mode", "Remote");
    string job_type = jsonGetString(payload, "job_type", "Full-time");
    string location = jsonGetString(payload, "location");
    string job_url = jsonGetString(payload, "job_url");
    string salary_min_str = jsonGetString(payload, "salary_min", "0");
    string salary_max_str = jsonGetString(payload, "salary_max", "0");
    string currency = jsonGetString(payload, "currency", "$");
    string salary_period = jsonGetString(payload, "salary_period", "Yearly");
    string date_applied = jsonGetString(payload, "date_applied");
    string next_interview = jsonGetString(payload, "next_interview");
    string follow_up_date = jsonGetString(payload, "follow_up_date");
    string contact_name = jsonGetString(payload, "contact_name");
    string contact_email = jsonGetString(payload, "contact_email");
    string contact_phone = jsonGetString(payload, "contact_phone");
    string notes = jsonGetString(payload, "notes");
    string tags = jsonGetString(payload, "tags");

    double salary_min = atof(salary_min_str.c_str());
    double salary_max = atof(salary_max_str.c_str());

    bool isNew = id.empty();
    if (isNew) {
        id = generateId();
        const char* sql = "INSERT INTO applications (id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags, created_at, updated_at) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now', 'localtime'), datetime('now', 'localtime'));";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, company.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, work_mode.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, job_type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, location.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 8, job_url.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 9, salary_min);
            sqlite3_bind_double(stmt, 10, salary_max);
            sqlite3_bind_text(stmt, 11, currency.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 12, salary_period.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 13, date_applied.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 14, next_interview.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 15, follow_up_date.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 16, contact_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 17, contact_email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 18, contact_phone.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 19, notes.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 20, tags.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            logActivity("CREATED", "Added application for " + role + " at " + company, id);
        }
    } else {
        const char* sql = "UPDATE applications SET company=?, role=?, status=?, work_mode=?, job_type=?, location=?, job_url=?, salary_min=?, salary_max=?, currency=?, salary_period=?, date_applied=?, next_interview=?, follow_up_date=?, contact_name=?, contact_email=?, contact_phone=?, notes=?, tags=?, updated_at=datetime('now', 'localtime') WHERE id=?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, company.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, work_mode.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, job_type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, location.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, job_url.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 8, salary_min);
            sqlite3_bind_double(stmt, 9, salary_max);
            sqlite3_bind_text(stmt, 10, currency.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 11, salary_period.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 12, date_applied.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 13, next_interview.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 14, follow_up_date.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 15, contact_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 16, contact_email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 17, contact_phone.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 18, notes.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 19, tags.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 20, id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            logActivity("UPDATED", "Updated application details for " + company, id);
        }
    }

    return "{\"success\":true,\"id\":\"" + id + "\"}";
}

// Delete application
string deleteApplication(const string& id) {
    const char* sql = "DELETE FROM applications WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        logActivity("DELETED", "Deleted application " + id, id);
    }
    return "{\"success\":true}";
}

// Update stage directly
string updateStage(const string& id, const string& newStage) {
    const char* sql = "UPDATE applications SET status = ?, updated_at = datetime('now', 'localtime') WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newStage.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        logActivity("STAGE_CHANGE", "Moved application to " + newStage, id);
    }
    return "{\"success\":true}";
}

// Export CSV string
string exportCsv() {
    const char* sql = "SELECT id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags FROM applications ORDER BY date_applied DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return "";

    ostringstream csv;
    csv << "ID,Company,Role,Status,WorkMode,JobType,Location,JobURL,SalaryMin,SalaryMax,Currency,Period,DateApplied,NextInterview,FollowUpDate,ContactName,ContactEmail,ContactPhone,Notes,Tags\r\n";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int c = 0; c < 20; c++) {
            if (c > 0) csv << ",";
            const char* val = (const char*)sqlite3_column_text(stmt, c);
            string s = val ? val : "";
            // Replace internal quotes
            string escaped = "";
            for (char ch : s) {
                if (ch == '"') escaped += "\"\"";
                else if (ch == '\r' || ch == '\n') escaped += " ";
                else escaped += ch;
            }
            csv << "\"" << escaped << "\"";
        }
        csv << "\r\n";
    }

    sqlite3_finalize(stmt);
    return csv.str();
}

// Read whole file from disk
string readFileContent(const string& filePath) {
    ifstream in(filePath, ios::in | ios::binary);
    if (!in.is_open()) return "";
    ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

// Get DB file size
long getFileSize(const string& filename) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(filename.c_str(), GetFileExInfoStandard, &fad)) return 0;
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return (long)size.QuadPart;
}

// Reliable socket send all bytes
void sendAll(SOCKET sock, const char* data, int total) {
    int sent = 0;
    while (sent < total) {
        int n = send(sock, data + sent, total - sent, 0);
        if (n <= 0) break;
        sent += n;
    }
}

// Minimal HTTP Server using Winsock
void runHttpServer() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "WSAStartup failed." << endl;
        return;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        return;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(g_port);

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Bind failed on port " << g_port << ". Trying 4830..." << endl;
        g_port = 4830;
        addr.sin_port = htons(g_port);
        if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            cerr << "Bind failed on alternate port. Exiting." << endl;
            closesocket(serverSock);
            WSACleanup();
            return;
        }
    }

    if (listen(serverSock, 10) == SOCKET_ERROR) {
        cerr << "Listen failed." << endl;
        closesocket(serverSock);
        WSACleanup();
        return;
    }

    SetConsoleTitleA("APPLY TRACK - Native Windows Job Application Tracker");
    cout << "==========================================================" << endl;
    cout << "  APPLY TRACK v1.0.0 - Native Windows Desktop App" << endl;
    cout << "  Server:   http://127.0.0.1:" << g_port << endl;
    cout << "  Storage:  " << g_dbPath << endl;
    cout << "==========================================================" << endl;
    cout << "  [OK] Launched application window." << endl;
    cout << "  [OK] Zero Node.js runtime / Embedded SQLite internal vault." << endl;
    cout << "  [TIP] Keep this console open, or press Ctrl+C to exit." << endl;
    cout << "==========================================================" << endl;

    // Launch Edge App mode window or default browser
    string url = "http://127.0.0.1:" + to_string(g_port);
    string edgeParam = "--app=" + url + " --window-size=1300,840";
    HINSTANCE hEdge = ShellExecuteA(NULL, "open", "msedge.exe", edgeParam.c_str(), NULL, SW_SHOWNORMAL);
    if ((intptr_t)hEdge <= 32) {
        // Fallback to default browser
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    // Server loop
    while (true) {
        SOCKET clientSock = accept(serverSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) break;

        char buffer[8192];
        int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            string request(buffer, bytesRead);

            // Parse HTTP Request line
            istringstream iss(request);
            string method, uri, httpVer;
            iss >> method >> uri >> httpVer;

            // Extract body if POST/PUT
            string body = "";
            size_t headerEnd = request.find("\r\n\r\n");
            if (headerEnd != string::npos) {
                body = request.substr(headerEnd + 4);
            }

            // Read remaining body based on Content-Length
            size_t clPos = request.find("Content-Length:");
            if (clPos == string::npos) clPos = request.find("content-length:");
            if (clPos != string::npos) {
                size_t numStart = clPos + 15;
                while (numStart < request.size() && (request[numStart] == ' ' || request[numStart] == '\t')) numStart++;
                int contentLength = atoi(request.c_str() + numStart);
                while ((int)body.size() < contentLength) {
                    int more = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                    if (more <= 0) break;
                    body.append(buffer, more);
                }
            }

            string responseHeader = "HTTP/1.1 200 OK\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\n";
            string responseBody = "";
            string contentType = "text/plain";

            if (method == "OPTIONS") {
                responseBody = "";
            } else if (method == "GET" && (uri == "/" || uri == "/index.html")) {
                string disk = readFileContent("src/renderer/index.html");
                responseBody = !disk.empty() ? disk : string((const char*)ASSET_INDEX_HTML_DATA, ASSET_INDEX_HTML_LEN);
                contentType = "text/html; charset=utf-8";
            } else if (method == "GET" && uri == "/styles.css") {
                string disk = readFileContent("src/renderer/styles.css");
                responseBody = !disk.empty() ? disk : string((const char*)ASSET_STYLES_CSS_DATA, ASSET_STYLES_CSS_LEN);
                contentType = "text/css; charset=utf-8";
            } else if (method == "GET" && uri == "/app.js") {
                string disk = readFileContent("src/renderer/app.js");
                responseBody = !disk.empty() ? disk : string((const char*)ASSET_APP_JS_DATA, ASSET_APP_JS_LEN);
                contentType = "application/javascript; charset=utf-8";
            } else if (method == "GET" && uri == "/icons.js") {
                string disk = readFileContent("src/renderer/icons.js");
                responseBody = !disk.empty() ? disk : string((const char*)ASSET_ICONS_JS_DATA, ASSET_ICONS_JS_LEN);
                contentType = "application/javascript; charset=utf-8";
            } else if (method == "GET" && uri == "/app_icon.png") {
                string disk = readFileContent("src/renderer/app_icon.png");
                responseBody = !disk.empty() ? disk : string((const char*)ASSET_APP_ICON_PNG_DATA, ASSET_APP_ICON_PNG_LEN);
                contentType = "image/png";
            } else if (method == "GET" && uri == "/apply_track_banner.png") {
                string disk = readFileContent("src/renderer/apply_track_banner.png");
                responseBody = !disk.empty() ? disk : string((const char*)ASSET_BANNER_PNG_DATA, ASSET_BANNER_PNG_LEN);
                contentType = "image/png";
            } else if (method == "GET" && uri == "/api/applications") {
                responseBody = getApplicationsJson();
                contentType = "application/json";
            } else if (method == "GET" && uri == "/api/activity") {
                responseBody = getActivityJson();
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/applications") {
                responseBody = saveApplicationFromPayload(body);
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/stage") {
                string id = jsonGetString(body, "id");
                string stage = jsonGetString(body, "stage");
                responseBody = updateStage(id, stage);
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/delete") {
                string id = jsonGetString(body, "id");
                responseBody = deleteApplication(id);
                contentType = "application/json";
            } else if (method == "GET" && uri == "/api/storage") {
                ostringstream s;
                s << "{"
                  << "\"storageDir\":\"" << jsonEscape(g_storageDir) << "\","
                  << "\"dbPath\":\"" << jsonEscape(g_dbPath) << "\","
                  << "\"defaultDir\":\"" << jsonEscape(getDefaultAppDataDir()) << "\","
                  << "\"isCustom\":" << (_stricmp(g_storageDir.c_str(), getDefaultAppDataDir().c_str()) != 0 ? "true" : "false") << ","
                  << "\"dbSize\":" << getFileSize(g_dbPath)
                  << "}";
                responseBody = s.str();
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/storage/location") {
                string targetDir = jsonGetString(body, "dir");
                bool moveExisting = jsonGetBool(body, "move_existing", true);
                string err = "";
                if (changeDatabaseLocation(targetDir, moveExisting, err)) {
                    logActivity("STORAGE_LOCATION", "Database folder updated to: " + g_storageDir);
                    ostringstream s;
                    s << "{"
                      << "\"success\":true,"
                      << "\"storageDir\":\"" << jsonEscape(g_storageDir) << "\","
                      << "\"dbPath\":\"" << jsonEscape(g_dbPath) << "\","
                      << "\"defaultDir\":\"" << jsonEscape(getDefaultAppDataDir()) << "\","
                      << "\"isCustom\":" << (_stricmp(g_storageDir.c_str(), getDefaultAppDataDir().c_str()) != 0 ? "true" : "false") << ","
                      << "\"dbSize\":" << getFileSize(g_dbPath)
                      << "}";
                    responseBody = s.str();
                } else {
                    responseBody = "{\"success\":false,\"error\":\"" + jsonEscape(err) + "\"}";
                }
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/storage/browse") {
                string selected = chooseFolderDialog();
                if (!selected.empty()) {
                    responseBody = "{\"success\":true,\"path\":\"" + jsonEscape(selected) + "\"}";
                } else {
                    responseBody = "{\"success\":false,\"cancelled\":true}";
                }
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/storage/open") {
                ShellExecuteA(NULL, "open", "explorer.exe", g_storageDir.c_str(), NULL, SW_SHOWNORMAL);
                responseBody = "{\"success\":true}";
                contentType = "application/json";
            } else if (method == "GET" && uri == "/api/export/csv") {
                responseBody = exportCsv();
                contentType = "text/csv; charset=utf-8";
                responseHeader += "Content-Disposition: attachment; filename=JobApplications.csv\r\n";
            } else if (method == "POST" && uri == "/api/clear") {
                sqlite3_exec(db, "DELETE FROM applications; DELETE FROM activity_log; VACUUM;", nullptr, nullptr, nullptr);
                responseBody = "{\"success\":true}";
                contentType = "application/json";
            } else if (method == "POST" && uri == "/api/exit") {
                responseBody = "{\"success\":true,\"message\":\"Application shutting down\"}";
                contentType = "application/json";
                responseHeader += "Content-Type: " + contentType + "\r\n";
                responseHeader += "Content-Length: " + to_string(responseBody.size()) + "\r\n\r\n";
                sendAll(clientSock, responseHeader.data(), (int)responseHeader.size());
                sendAll(clientSock, responseBody.data(), (int)responseBody.size());
                shutdown(clientSock, SD_BOTH);
                closesocket(clientSock);
                closesocket(serverSock);
                WSACleanup();
                if (db) sqlite3_close(db);
                exit(0);
            } else {
                responseHeader = "HTTP/1.1 404 Not Found\r\n";
                responseBody = "Not found";
            }

            responseHeader += "Content-Type: " + contentType + "\r\n";
            responseHeader += "Content-Length: " + to_string(responseBody.size()) + "\r\n\r\n";

            sendAll(clientSock, responseHeader.data(), (int)responseHeader.size());
            if (!responseBody.empty()) {
                sendAll(clientSock, responseBody.data(), (int)responseBody.size());
            }
            shutdown(clientSock, SD_SEND);
        }
        closesocket(clientSock);
    }

    closesocket(serverSock);
    WSACleanup();
}

int main() {
    if (!initDatabase()) {
        cerr << "Failed to initialize Windows internal database. Exiting." << endl;
        return 1;
    }

    runHttpServer();

    if (db) sqlite3_close(db);
    return 0;
}
