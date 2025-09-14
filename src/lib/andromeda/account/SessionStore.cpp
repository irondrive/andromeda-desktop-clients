
#include "Session.hpp"
#include "SessionStore.hpp"

#include "andromeda/database/MixedValue.hpp"
using Andromeda::Database::MixedParams;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;
#include "andromeda/database/TableBuilder.hpp"
using Andromeda::Database::TableBuilder;

namespace Andromeda {
namespace Account {

/*****************************************************/
SessionStore::SessionStore(ObjectDatabase& database, const MixedParams& data, bool created) :
    BaseObject(database),
    mServerUrl("serverUrl",*this),
    mSessionID("sessionID",*this),
    mSessionKey("sessionKey",*this)
{
    RegisterFields({&mServerUrl, &mSessionID, &mSessionKey});
    InitializeFields(data, created);
}

/*****************************************************/
TableBuilder SessionStore::GetTableInstall()
{
    TableBuilder tb { TableBuilder::For<SessionStore>() };
    tb.AddColumn("id","varchar(12)",false).SetPrimary("id")
      .AddColumn("serverUrl","text",false)
      //.AddColumn("accountID","char(12)",false).AddUnique("accountID") // TODO unique field?
      .AddColumn("sessionID","char(12)",false)
      .AddColumn("sessionKey","char(32)",false);
    return tb;
}

/*****************************************************/
TableBuilder SessionStore::GetTableUpgrade(int newVersion)
{
    return TableBuilder::For<SessionStore>(); // empty
}

/*****************************************************/
SessionStore& SessionStore::Create(ObjectDatabase& db, const std::string& serverUrl, const Session& session)
{
    SessionStore& obj { db.CreateObject<SessionStore>() };
    obj.mServerUrl = serverUrl;
    obj.mSessionID = session.GetSessionID();
    obj.mSessionKey = session.GetSessionKey();
    return obj;
}

/*****************************************************/
std::list<SessionStore*> SessionStore::LoadAll(ObjectDatabase& db) // cppcheck-suppress constParameterReference
{
    return db.LoadObjectsByQuery<SessionStore>({}); // empty WHERE
}

} // namespace Account
} // namespace Andromeda
