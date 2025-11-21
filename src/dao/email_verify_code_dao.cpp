#include "dao/email_verify_code_dao.hpp"

#include <mysql/mysql.h>

#include "base/macro.hpp"
#include "db/mysql.hpp"

namespace IM::dao {

static constexpr const char* kDBName = "default";

bool EmailVerifyCodeDAO::Create(const EmailVerifyCode& code, std::string* err) {
    auto db = IM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) {
        if (err) *err = "get mysql connection failed";
        return false;
    }

    const char* sql =
        "INSERT INTO im_email_verify_code (email, channel, code, status, sent_ip, sent_at, "
        "expire_at, used_at, created_at) VALUES (?, ?, ?, ?, ?, NOW(), ?, ?, NOW())";
    auto stmt = db->prepare(sql);
    if (!stmt) {
        if (err) *err = "prepare sql failed";
        return false;
    }
    stmt->bindString(1, code.email);
    stmt->bindString(2, code.channel);
    stmt->bindString(3, code.code);
    stmt->bindUint8(4, code.status);
    if (!code.sent_ip.empty())
        stmt->bindString(5, code.sent_ip);
    else
        stmt->bindNull(5);
    stmt->bindTime(6, code.expire_at);
    if (code.used_at != 0)
        stmt->bindTime(7, code.used_at);
    else
        stmt->bindNull(7);

    if (stmt->execute() != 0) {
        if (err) *err = stmt->getErrStr();
        return false;
    }
    return true;
}

bool EmailVerifyCodeDAO::Verify(const std::string& email, const std::string& code,
                                const std::string& channel, std::string* err) {
    auto db = IM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) {
        if (err) *err = "get mysql connection failed";
        return false;
    }

    const char* sql =
        "SELECT id FROM im_email_verify_code WHERE email = ? AND code = ? AND channel = ? AND "
        "status = 1 AND expire_at > NOW() ORDER BY created_at DESC LIMIT 1";
    auto stmt = db->prepare(sql);
    if (!stmt) {
        if (err) *err = "prepare sql failed";
        return false;
    }
    stmt->bindString(1, email);
    stmt->bindString(2, code);
    stmt->bindString(3, channel);
    auto res = stmt->query();
    if (!res) {
        if (err) *err = "query failed";
        return false;
    }

    if (!res->next()) {
        if (err) *err = "no record found";
        return false;
    }

    uint64_t id = res->getUint64(0);

    return MarkAsUsed(id, err);
}

bool EmailVerifyCodeDAO::MarkAsUsed(const uint64_t id, std::string* err) {
    auto db = IM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) {
        if (err) *err = "get mysql connection failed";
        return false;
    }

    const char* sql = "UPDATE im_email_verify_code SET status = 2, used_at = NOW() WHERE id = ?";
    auto stmt = db->prepare(sql);
    if (!stmt) {
        if (err) *err = "prepare sql failed";
        return false;
    }
    stmt->bindUint64(1, id);
    if (stmt->execute() != 0) {
        if (err) *err = stmt->getErrStr();
        return false;
    }
    return true;
}

bool EmailVerifyCodeDAO::MarkExpiredAsInvalid(std::string* err) {
    auto db = IM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) {
        if (err) *err = "get mysql connection failed";
        return false;
    }
    const char* sql =
        "UPDATE im_email_verify_code SET status = 3 WHERE expire_at < NOW() AND status = 1";
    auto stmt = db->prepare(sql);
    if (!stmt) {
        if (err) *err = "prepare sql failed";
        return false;
    }
    if (stmt->execute() != 0) {
        if (err) *err = stmt->getErrStr();
        return false;
    }
    return true;
}

bool EmailVerifyCodeDAO::DeleteInvalidCodes(std::string* err) {
    auto db = IM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) {
        if (err) *err = "get mysql connection failed";
        return false;
    }

    const char* sql = "DELETE FROM im_email_verify_code WHERE status = 3";
    auto stmt = db->prepare(sql);
    if (!stmt) {
        if (err) *err = "prepare sql failed";
        return false;
    }
    if (stmt->execute() != 0) {
        if (err) *err = stmt->getErrStr();
        return false;
    }
    return true;
}

}  // namespace IM::dao
