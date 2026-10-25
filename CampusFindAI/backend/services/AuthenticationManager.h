#ifndef AUTHENTICATION_MANAGER_H
#define AUTHENTICATION_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include "../models/User.h"
#include "FileManager.h"

// AuthenticationManager
// ----------------------
// Simple username(email)/password login backed by whichever storage backend
// is passed in (FileManager or DatabaseManager — both expose the same
// saveUser()/loadUsers() interface, so this class is templated on the
// storage type rather than hard-wired to one). Passwords are hashed with
// std::hash (fine for an academic project; never stored plain, never
// returned to the frontend). Provides role checks used to enforce
// admin-only actions (Section 42 - Security).
template <typename Storage>
class AuthenticationManager {
public:
    explicit AuthenticationManager(Storage& fm) : fileManager(fm) {
        users = fileManager.loadUsers();
        if (users.empty()) seedDefaultUsers();
    }

    static std::string hashPassword(const std::string& plain) {
        std::hash<std::string> hasher;
        return std::to_string(hasher(plain + "campusfind_salt"));
    }

    // Returns userId on success, empty string on failure.
    std::string login(const std::string& email, const std::string& password) {
        std::string hashed = hashPassword(password);
        for (auto& u : users) {
            if (u.email == email && u.passwordHash == hashed) return u.userId;
        }
        return "";
    }

    bool userExists(const std::string& email) const {
        for (auto& u : users) if (u.email == email) return true;
        return false;
    }

    std::string registerUser(const std::string& name, const std::string& email,
                              const std::string& password, Role role,
                              std::function<std::string()> idGen) {
        if (userExists(email)) return "";
        std::string id = idGen();
        User u(id, name, email, hashPassword(password), role);
        users.push_back(u);
        fileManager.saveUser(u);
        return id;
    }

    User* findById(const std::string& userId) {
        for (auto& u : users) if (u.userId == userId) return &u;
        return nullptr;
    }

    bool isAdmin(const std::string& userId) {
        User* u = findById(userId);
        return u && u->role == Role::ADMIN;
    }

    std::vector<User>& all() { return users; }

private:
    Storage& fileManager;
    std::vector<User> users;

    void seedDefaultUsers() {
        User admin("USR-2026-0001", "System Admin", "admin@campusfind.edu",
                    hashPassword("admin123"), Role::ADMIN);
        User student("USR-2026-0002", "Aditi Sharma", "aditi@campusfind.edu",
                      hashPassword("student123"), Role::STUDENT);
        users.push_back(admin);
        users.push_back(student);
        fileManager.saveUser(admin);
        fileManager.saveUser(student);
    }
};

#endif // AUTHENTICATION_MANAGER_H
