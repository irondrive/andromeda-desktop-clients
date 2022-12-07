#ifndef A2GUI_BACKENDCONTEXT_H
#define A2GUI_BACKENDCONTEXT_H

#include <memory>

#include "andromeda/BackendOptions.hpp"
#include "andromeda/HTTPRunnerOptions.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda { class Backend; class HTTPRunner; }

/** Encapsulates a backend and its resources */
class BackendContext
{
public:

    /** Create a new BackendContext from user input */
    BackendContext(
        const std::string& url, const std::string& username, 
        const std::string& password, const std::string& twofactor);

    virtual ~BackendContext();

    /** Returns the Andromeda::Backend instance */
    Andromeda::Backend& GetBackend() { return *mBackend; }

private:

    /** libandromeda configuration */
    Andromeda::BackendOptions mConfigOptions;
    /** HTTP Runner configuration */
    Andromeda::HTTPRunnerOptions mHttpOptions;
    
    std::unique_ptr<Andromeda::HTTPRunner> mRunner;
    std::unique_ptr<Andromeda::Backend> mBackend;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_BACKENDCONTEXT_H
