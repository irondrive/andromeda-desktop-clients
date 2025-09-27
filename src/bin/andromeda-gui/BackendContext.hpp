#ifndef A2GUI_BACKENDCONTEXT_H
#define A2GUI_BACKENDCONTEXT_H

#include <memory>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"
#include "andromeda/backend/RunnerOptions.hpp"

namespace Andromeda { 
    class SecureBuffer;
    namespace Account { class Session; class SessionStore; }
    namespace Backend { 
        class BackendImpl; class HTTPRunner; class RunnerPool; }
    namespace Database { class ObjectDatabase; } }

namespace AndromedaGui {

/** Encapsulates a backend and its resources */
class BackendContext
{
public:

    /** 
     * Create a new BackendContext from user input (sessionstore is nullptr)
     * @throws BackendException for backend issues
     */
    BackendContext(const std::string& url, const std::string& username, 
        const Andromeda::SecureBuffer& password, const std::string& twofactor);

    /** 
     * Create a new BackendContext from a known session and store ref
     * @throws BackendException for backend issues
     */
    explicit BackendContext(Andromeda::Account::SessionStore& session);

    virtual ~BackendContext();
    DELETE_COPY(BackendContext)
    DELETE_MOVE(BackendContext)
    
    /** 
     * Return the hostname_username ID string 
     * @param human if true make it human-pretty
     */
    [[nodiscard]] std::string GetName(bool human) const;

    /** Returns the Backend instance */
    inline Andromeda::Backend::BackendImpl& GetBackend() { return *mBackend; }
    /** Returns the HTTPRunner instance */
    inline Andromeda::Backend::HTTPRunner& GetRunner() { return *mRunner; }

    /** 
     * Creates a new session store and stores it here
     * @throws DatabaseException
     */
    void StoreSession(Andromeda::Database::ObjectDatabase& objdb);
    /** Returns the SessionStore instance or nullptr if not set */
    inline Andromeda::Account::SessionStore* GetSessionStore() const { return mSessionStore; }

private:

    /** Create the backend and runner objects */
    void InitializeBackend(const std::string& url);

    /** Returns the andromeda-gui http user agent */
    static std::string GetUserAgent();

    mutable Andromeda::Debug mDebug;

    /** libandromeda configuration */
    Andromeda::ConfigOptions mConfigOptions;
    /** HTTP Runner configuration */
    Andromeda::Backend::HTTPOptions mHttpOptions;
    Andromeda::Backend::RunnerOptions mRunnerOptions;
    
    std::unique_ptr<Andromeda::Backend::HTTPRunner> mRunner; // NEVER NULL
    std::unique_ptr<Andromeda::Backend::RunnerPool> mRunners; // NEVER NULL
    std::unique_ptr<Andromeda::Backend::BackendImpl> mBackend; // NEVER NULL
    std::unique_ptr<Andromeda::Account::Session> mSession; // NEVER NULL

    Andromeda::Account::SessionStore* mSessionStore { nullptr };
};

} // namespace AndromedaGui

#endif // A2GUI_BACKENDCONTEXT_H
