#ifndef LIBA2_SESSIONSTORE_H_
#define LIBA2_SESSIONSTORE_H_

#include <cstddef>
#include <list>
#include <string>

#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/TableBuilder.hpp"
#include "andromeda/database/fieldtypes/ScalarType.hpp"

namespace Andromeda {
namespace Backend { class BackendImpl; }
namespace Account { class Session;

/** Stores an account and session in the database */
class SessionStore : public Database::BaseObject
{
public:

    // BaseObject functions
    BASEOBJECT_NAME(SessionStore, "Andromeda\\Database\\SessionStore")
    SessionStore(Database::ObjectDatabase& database, const Database::MixedParams& data, bool created);

    // TableInstaller functions
    [[nodiscard]] inline static int GetTableVersion() { return 1; }
    static Database::TableBuilder GetTableInstall();
    static Database::TableBuilder GetTableUpgrade(int newVersion);

    /** 
     * Create a new session store for the given server URL and session
     * @param db reference to the ObjectDatabase
     * @param serverURL the URL to the server to store
     * @param session session object to create an entry for
     */
    static SessionStore& Create(Database::ObjectDatabase& db, const std::string& serverUrl, const Session& session); 

    /** 
     * Loads a list of non-null pointers to all saved sessions 
     * @throws DatabaseException
     */
    static std::list<SessionStore*> LoadAll(Database::ObjectDatabase& db);

    /** Returns the server URL this session is for */
    inline const std::string& GetServerUrl() const { return mServerUrl.GetValue(); }
    /** Returns the stored session ID */
    inline const std::string& GetSessionID() const { return mSessionID.GetValue(); }
    /** Returns the stored session key */
    inline const std::string& GetSessionKey() const { return mSessionKey.GetValue(); }
    /** Returns the stored e2ee master key or nullptr if not set */
    inline const SecureBuffer* TryGetMasterKey() const { return mE2eeMaster.TryGetValue(); }

    /** Sets the stored e2ee master key (will copy), clears if nullptr */
    inline bool SetMasterKey(const SecureBuffer& key) { return mE2eeMaster.SetValue(key); }

private:

    Database::FieldTypes::ScalarType<std::string> mServerUrl;
    Database::FieldTypes::ScalarType<std::string> mSessionID;
    Database::FieldTypes::ScalarType<std::string> mSessionKey;
    Database::FieldTypes::NullScalarType<SecureBuffer> mE2eeMaster; 
    // TODO E2EE use libkeychain instead for e2ee key (and session key?)
    // then can remove SecureBuffer from MixedValue which is iffy anyway
};

} // namespace Account
} // namespace Andromeda

#endif // LIBA2_SESSIONSTORE_H_
