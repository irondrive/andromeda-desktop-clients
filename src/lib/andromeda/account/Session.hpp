#ifndef LIBA2_SESSION_H_
#define LIBA2_SESSION_H_

#include "andromeda/common.hpp"

namespace Andromeda {
namespace Account {

// TODO RAY !! add comments
class Session
{
public:

    // TODO RAY !! add init functions - from SessionStore, Authenticate, AuthenticateInteractive, load from SessionOptions...

    virtual ~Session();
    DELETE_COPY(Session)
    DELETE_MOVE(Session)

private:

};

} // namespace Account
} // namespace Andromeda

#endif // LIBA2_SESSION_H_
