#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <sys/stat.h>
#include <sqlite3.h>
#include "../models/User.h"
#include "../models/LostItem.h"
#include "../models/FoundItem.h"
#include "../models/Claim.h"
#include "../models/Notification.h"
#include "../models/HistoryRecord.h"

// DatabaseManager
// ---------------
// Real SQL database persistence via SQLite (embedded, single-file, no
// server process required — the whole database lives in one .db file that
// can be opened directly in "DB Browser for SQLite" or any SQL client to
// show real tables/rows to an examiner).
//
// This class exposes EXACTLY the same public methods as FileManager
// (saveLostItem, loadLostItems, rewriteLostItems, ...), so it is a drop-in
// replacement: main.cpp only needs to change
//     FileManager fileManager("data");
// to
//     DatabaseManager fileManager("data/campusfind.db");
// Every other line of business logic that calls fileManager.* is untouched.
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbPath) {
        // Ensure the containing directory exists (mirrors FileManager's
        // behaviour of creating data/ on first run).
        std::string dir = dbPath.substr(0, dbPath.find_last_of("/\\"));
        if (!dir.empty()) {
            struct stat st{};
            if (stat(dir.c_str(), &st) != 0) {
#ifdef _WIN32
                mkdir(dir.c_str());
#else
                mkdir(dir.c_str(), 0755);
#endif
            }
        }

        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error(std::string("Could not open database: ") + sqlite3_errmsg(db));
        }
        exec("PRAGMA journal_mode=WAL;"); // safer concurrent read/write, standard SQLite practice
        createSchema();
    }

    ~DatabaseManager() {
        if (db) sqlite3_close(db);
    }

    // ---------------- Users ----------------
    bool saveUser(const User& u) {
        const char* sql = "INSERT OR REPLACE INTO users (userId,name,email,passwordHash,role) VALUES (?,?,?,?,?);";
        sqlite3_stmt* stmt = prepare(sql);
        bindText(stmt, 1, u.userId); bindText(stmt, 2, u.name); bindText(stmt, 3, u.email);
        bindText(stmt, 4, u.passwordHash); bindText(stmt, 5, roleToString(u.role));
        return step(stmt);
    }

    std::vector<User> loadUsers() {
        std::vector<User> out;
        sqlite3_stmt* stmt = prepare("SELECT userId,name,email,passwordHash,role FROM users;");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            out.push_back(User(col(stmt,0), col(stmt,1), col(stmt,2), col(stmt,3), roleFromString(col(stmt,4))));
        }
        sqlite3_finalize(stmt);
        return out;
    }

    bool rewriteUsers(const std::vector<User>& users) {
        exec("DELETE FROM users;");
        for (auto& u : users) saveUser(u);
        return true;
    }

    // ---------------- Lost Items ----------------
    bool saveLostItem(const LostItem& it) {
        const char* sql = "INSERT OR REPLACE INTO lost_items "
            "(itemId,reporterId,itemName,category,color,brand,location,dateText,timeText,description,"
            "uniqueFeature,contactPreference,status,createdAt,imagePath,imageHash) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
        sqlite3_stmt* stmt = prepare(sql);
        bindText(stmt,1,it.itemId); bindText(stmt,2,it.reporterId); bindText(stmt,3,it.itemName);
        bindText(stmt,4,it.category); bindText(stmt,5,it.color); bindText(stmt,6,it.brand);
        bindText(stmt,7,it.location); bindText(stmt,8,it.dateText); bindText(stmt,9,it.timeText);
        bindText(stmt,10,it.description); bindText(stmt,11,it.uniqueFeature); bindText(stmt,12,it.contactPreference);
        bindText(stmt,13,it.status); bindText(stmt,14,it.createdAt); bindText(stmt,15,it.imagePath); bindText(stmt,16,it.imageHash);
        return step(stmt);
    }

    std::vector<LostItem> loadLostItems() {
        std::vector<LostItem> out;
        sqlite3_stmt* stmt = prepare("SELECT itemId,reporterId,itemName,category,color,brand,location,dateText,"
            "timeText,description,uniqueFeature,contactPreference,status,createdAt,imagePath,imageHash FROM lost_items;");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LostItem it;
            it.itemId=col(stmt,0); it.reporterId=col(stmt,1); it.itemName=col(stmt,2); it.category=col(stmt,3);
            it.color=col(stmt,4); it.brand=col(stmt,5); it.location=col(stmt,6); it.dateText=col(stmt,7);
            it.timeText=col(stmt,8); it.description=col(stmt,9); it.uniqueFeature=col(stmt,10);
            it.contactPreference=col(stmt,11); it.status=col(stmt,12); it.createdAt=col(stmt,13);
            it.imagePath=col(stmt,14); it.imageHash=col(stmt,15);
            out.push_back(it);
        }
        sqlite3_finalize(stmt);
        return out;
    }

    bool rewriteLostItems(const std::vector<LostItem>& items) {
        exec("DELETE FROM lost_items;");
        for (auto& it : items) saveLostItem(it);
        return true;
    }

    // ---------------- Found Items ----------------
    bool saveFoundItem(const FoundItem& it) {
        const char* sql = "INSERT OR REPLACE INTO found_items "
            "(itemId,reporterId,itemName,category,color,brand,location,dateText,timeText,description,"
            "uniqueFeature,storageLocation,status,createdAt,imagePath,imageHash) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
        sqlite3_stmt* stmt = prepare(sql);
        bindText(stmt,1,it.itemId); bindText(stmt,2,it.reporterId); bindText(stmt,3,it.itemName);
        bindText(stmt,4,it.category); bindText(stmt,5,it.color); bindText(stmt,6,it.brand);
        bindText(stmt,7,it.location); bindText(stmt,8,it.dateText); bindText(stmt,9,it.timeText);
        bindText(stmt,10,it.description); bindText(stmt,11,it.uniqueFeature); bindText(stmt,12,it.storageLocation);
        bindText(stmt,13,it.status); bindText(stmt,14,it.createdAt); bindText(stmt,15,it.imagePath); bindText(stmt,16,it.imageHash);
        return step(stmt);
    }

    std::vector<FoundItem> loadFoundItems() {
        std::vector<FoundItem> out;
        sqlite3_stmt* stmt = prepare("SELECT itemId,reporterId,itemName,category,color,brand,location,dateText,"
            "timeText,description,uniqueFeature,storageLocation,status,createdAt,imagePath,imageHash FROM found_items;");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            FoundItem it;
            it.itemId=col(stmt,0); it.reporterId=col(stmt,1); it.itemName=col(stmt,2); it.category=col(stmt,3);
            it.color=col(stmt,4); it.brand=col(stmt,5); it.location=col(stmt,6); it.dateText=col(stmt,7);
            it.timeText=col(stmt,8); it.description=col(stmt,9); it.uniqueFeature=col(stmt,10);
            it.storageLocation=col(stmt,11); it.status=col(stmt,12); it.createdAt=col(stmt,13);
            it.imagePath=col(stmt,14); it.imageHash=col(stmt,15);
            out.push_back(it);
        }
        sqlite3_finalize(stmt);
        return out;
    }

    bool rewriteFoundItems(const std::vector<FoundItem>& items) {
        exec("DELETE FROM found_items;");
        for (auto& it : items) saveFoundItem(it);
        return true;
    }

    // ---------------- Claims ----------------
    bool saveClaim(const Claim& c) {
        std::ostringstream verif;
        bool first = true;
        for (auto& kv : c.verificationAnswers) {
            if (!first) verif << ";;";
            verif << kv.first << "::" << kv.second;
            first = false;
        }
        const char* sql = "INSERT OR REPLACE INTO claims "
            "(claimId,lostItemId,foundItemId,claimantId,claimDate,matchScore,ownershipScore,status,"
            "administratorId,decisionDate,remarks,verificationAnswers) VALUES (?,?,?,?,?,?,?,?,?,?,?,?);";
        sqlite3_stmt* stmt = prepare(sql);
        bindText(stmt,1,c.claimId); bindText(stmt,2,c.lostItemId); bindText(stmt,3,c.foundItemId);
        bindText(stmt,4,c.claimantId); bindText(stmt,5,c.claimDate);
        sqlite3_bind_int(stmt,6,c.matchScore); sqlite3_bind_int(stmt,7,c.ownershipScore);
        bindText(stmt,8,claimStatusToString(c.status)); bindText(stmt,9,c.administratorId);
        bindText(stmt,10,c.decisionDate); bindText(stmt,11,c.remarks); bindText(stmt,12,verif.str());
        return step(stmt);
    }

    std::vector<Claim> loadClaims() {
        std::vector<Claim> out;
        sqlite3_stmt* stmt = prepare("SELECT claimId,lostItemId,foundItemId,claimantId,claimDate,matchScore,"
            "ownershipScore,status,administratorId,decisionDate,remarks,verificationAnswers FROM claims;");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Claim c;
            c.claimId=col(stmt,0); c.lostItemId=col(stmt,1); c.foundItemId=col(stmt,2); c.claimantId=col(stmt,3);
            c.claimDate=col(stmt,4); c.matchScore=sqlite3_column_int(stmt,5); c.ownershipScore=sqlite3_column_int(stmt,6);
            c.status=claimStatusFromString(col(stmt,7)); c.administratorId=col(stmt,8); c.decisionDate=col(stmt,9);
            c.remarks=col(stmt,10);
            std::string raw = col(stmt,11);
            size_t pos = 0;
            while (pos < raw.size()) {
                size_t next = raw.find(";;", pos);
                std::string chunk = (next == std::string::npos) ? raw.substr(pos) : raw.substr(pos, next - pos);
                size_t sep = chunk.find("::");
                if (sep != std::string::npos) c.verificationAnswers[chunk.substr(0,sep)] = chunk.substr(sep+2);
                if (next == std::string::npos) break;
                pos = next + 2;
            }
            out.push_back(c);
        }
        sqlite3_finalize(stmt);
        return out;
    }

    bool rewriteClaims(const std::vector<Claim>& claims) {
        exec("DELETE FROM claims;");
        for (auto& c : claims) saveClaim(c);
        return true;
    }

    // ---------------- Notifications ----------------
    bool saveNotification(const Notification& n) {
        const char* sql = "INSERT OR REPLACE INTO notifications (notifId,userId,title,message,relatedId,createdAt,isRead) VALUES (?,?,?,?,?,?,?);";
        sqlite3_stmt* stmt = prepare(sql);
        bindText(stmt,1,n.notifId); bindText(stmt,2,n.userId); bindText(stmt,3,n.title);
        bindText(stmt,4,n.message); bindText(stmt,5,n.relatedId); bindText(stmt,6,n.createdAt);
        sqlite3_bind_int(stmt,7,n.isRead ? 1 : 0);
        return step(stmt);
    }

    std::vector<Notification> loadNotifications() {
        std::vector<Notification> out;
        sqlite3_stmt* stmt = prepare("SELECT notifId,userId,title,message,relatedId,createdAt,isRead FROM notifications;");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Notification n;
            n.notifId=col(stmt,0); n.userId=col(stmt,1); n.title=col(stmt,2); n.message=col(stmt,3);
            n.relatedId=col(stmt,4); n.createdAt=col(stmt,5); n.isRead = sqlite3_column_int(stmt,6) != 0;
            out.push_back(n);
        }
        sqlite3_finalize(stmt);
        return out;
    }

    bool rewriteNotifications(const std::vector<Notification>& notifs) {
        exec("DELETE FROM notifications;");
        for (auto& n : notifs) saveNotification(n);
        return true;
    }

    // ---------------- History ----------------
    bool saveHistory(const HistoryRecord& h) {
        const char* sql = "INSERT OR REPLACE INTO history (recordId,itemId,relatedId,eventType,eventText,timestamp) VALUES (?,?,?,?,?,?);";
        sqlite3_stmt* stmt = prepare(sql);
        bindText(stmt,1,h.recordId); bindText(stmt,2,h.itemId); bindText(stmt,3,h.relatedId);
        bindText(stmt,4,h.eventType); bindText(stmt,5,h.eventText); bindText(stmt,6,h.timestamp);
        return step(stmt);
    }

    std::vector<HistoryRecord> loadHistory() {
        std::vector<HistoryRecord> out;
        sqlite3_stmt* stmt = prepare("SELECT recordId,itemId,relatedId,eventType,eventText,timestamp FROM history;");
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            HistoryRecord h;
            h.recordId=col(stmt,0); h.itemId=col(stmt,1); h.relatedId=col(stmt,2);
            h.eventType=col(stmt,3); h.eventText=col(stmt,4); h.timestamp=col(stmt,5);
            out.push_back(h);
        }
        sqlite3_finalize(stmt);
        return out;
    }

private:
    sqlite3* db = nullptr;

    void exec(const std::string& sql) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "[DatabaseManager] SQL error: " << (errMsg ? errMsg : "unknown") << "\n";
            sqlite3_free(errMsg);
        }
    }

    sqlite3_stmt* prepare(const char* sql) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Prepare failed: " << sqlite3_errmsg(db) << "\n";
        }
        return stmt;
    }

    static void bindText(sqlite3_stmt* stmt, int idx, const std::string& val) {
        sqlite3_bind_text(stmt, idx, val.c_str(), -1, SQLITE_TRANSIENT);
    }

    static bool step(sqlite3_stmt* stmt) {
        bool ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return ok;
    }

    static std::string col(sqlite3_stmt* stmt, int idx) {
        const unsigned char* text = sqlite3_column_text(stmt, idx);
        return text ? reinterpret_cast<const char*>(text) : "";
    }

    void createSchema() {
        exec("CREATE TABLE IF NOT EXISTS users ("
             "userId TEXT PRIMARY KEY, name TEXT, email TEXT, passwordHash TEXT, role TEXT);");
        exec("CREATE TABLE IF NOT EXISTS lost_items ("
             "itemId TEXT PRIMARY KEY, reporterId TEXT, itemName TEXT, category TEXT, color TEXT, brand TEXT, "
             "location TEXT, dateText TEXT, timeText TEXT, description TEXT, uniqueFeature TEXT, "
             "contactPreference TEXT, status TEXT, createdAt TEXT, imagePath TEXT, imageHash TEXT);");
        exec("CREATE TABLE IF NOT EXISTS found_items ("
             "itemId TEXT PRIMARY KEY, reporterId TEXT, itemName TEXT, category TEXT, color TEXT, brand TEXT, "
             "location TEXT, dateText TEXT, timeText TEXT, description TEXT, uniqueFeature TEXT, "
             "storageLocation TEXT, status TEXT, createdAt TEXT, imagePath TEXT, imageHash TEXT);");
        exec("CREATE TABLE IF NOT EXISTS claims ("
             "claimId TEXT PRIMARY KEY, lostItemId TEXT, foundItemId TEXT, claimantId TEXT, claimDate TEXT, "
             "matchScore INTEGER, ownershipScore INTEGER, status TEXT, administratorId TEXT, decisionDate TEXT, "
             "remarks TEXT, verificationAnswers TEXT, "
             "FOREIGN KEY(lostItemId) REFERENCES lost_items(itemId), "
             "FOREIGN KEY(foundItemId) REFERENCES found_items(itemId));");
        exec("CREATE TABLE IF NOT EXISTS notifications ("
             "notifId TEXT PRIMARY KEY, userId TEXT, title TEXT, message TEXT, relatedId TEXT, "
             "createdAt TEXT, isRead INTEGER);");
        exec("CREATE TABLE IF NOT EXISTS history ("
             "recordId TEXT PRIMARY KEY, itemId TEXT, relatedId TEXT, eventType TEXT, eventText TEXT, timestamp TEXT);");
    }
};

#endif // DATABASE_MANAGER_H
