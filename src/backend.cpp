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

// Generate simple unique ID
string generateId() {
    static int counter = 0;
    counter++;
    ostringstream oss;
    oss << "app_" << (unsigned long)time(nullptr) << "_" << GetTickCount() << "_" << counter;
    return oss.str();
}

// Initialize SQLite in Windows Internal Storage
bool initDatabase() {
    char* appdata = getenv("APPDATA");
    if (!appdata) {
        char path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path) == S_OK) {
            g_storageDir = string(path) + "\\JobApplicationTracker";
        } else {
            g_storageDir = ".\\JobApplicationTracker";
        }
    } else {
        g_storageDir = string(appdata) + "\\JobApplicationTracker";
    }

    CreateDirectoryA(g_storageDir.c_str(), NULL);
    string attachDir = g_storageDir + "\\attachments";
    CreateDirectoryA(attachDir.c_str(), NULL);
    string backupDir = g_storageDir + "\\backups";
    CreateDirectoryA(backupDir.c_str(), NULL);

    g_dbPath = g_storageDir + "\\jobtracker.db";

    if (sqlite3_open(g_dbPath.c_str(), &db) != SQLITE_OK) {
        cerr << "Failed to open SQLite database: " << sqlite3_errmsg(db) << endl;
        return false;
    }

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
        cerr << "SQLite table init error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
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

    cout << "==========================================================" << endl;
    cout << "  JobTrack Minimal C++ Backend & Glassmorphic Server" << endl;
    cout << "  Listening on: http://127.0.0.1:" << g_port << endl;
    cout << "  SQLite Database: " << g_dbPath << endl;
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
                responseBody = readFileContent("src/renderer/index.html");
                contentType = "text/html; charset=utf-8";
            } else if (method == "GET" && uri == "/styles.css") {
                responseBody = readFileContent("src/renderer/styles.css");
                contentType = "text/css; charset=utf-8";
            } else if (method == "GET" && uri == "/app.js") {
                responseBody = readFileContent("src/renderer/app.js");
                contentType = "application/javascript; charset=utf-8";
            } else if (method == "GET" && uri == "/icons.js") {
                responseBody = readFileContent("src/renderer/icons.js");
                contentType = "application/javascript; charset=utf-8";
            } else if (method == "GET" && uri == "/app_icon.png") {
                responseBody = readFileContent("src/renderer/app_icon.png");
                contentType = "image/png";
            } else if (method == "GET" && uri == "/apply_track_banner.png") {
                responseBody = readFileContent("src/renderer/apply_track_banner.png");
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
                  << "\"dbSize\":" << getFileSize(g_dbPath)
                  << "}";
                responseBody = s.str();
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
            } else {
                responseHeader = "HTTP/1.1 404 Not Found\r\n";
                responseBody = "Not found";
            }

            responseHeader += "Content-Type: " + contentType + "\r\n";
            responseHeader += "Content-Length: " + to_string(responseBody.size()) + "\r\n\r\n";

            send(clientSock, responseHeader.c_str(), (int)responseHeader.size(), 0);
            if (!responseBody.empty()) {
                send(clientSock, responseBody.c_str(), (int)responseBody.size(), 0);
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
