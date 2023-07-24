#ifndef LIBA2_FIELDTYPES_H_
#define LIBA2_FIELDTYPES_H_

#include <string>

#include "andromeda/BaseException.hpp"
#include "andromeda/Utilities.hpp"
#include "andromeda/database/MixedValue.hpp"

namespace Andromeda {
namespace Database {

class BaseObject;
class ObjectDatabase;

namespace FieldTypes {

/** 
 * Base class representing a database column ("field") 
 * Mostly ported from the server's PHP implementation, scalar types are combined
 */
class BaseField
{
public:
    virtual ~BaseField() = default;
    DELETE_COPY(BaseField)
    DELETE_MOVE(BaseField)

    /** Exception indicating an uninitialized non-null field was accessed */
    class UninitializedException : public BaseException { public:
        explicit UninitializedException(const std::string& name) :
            BaseException("Uninitialized Field: "+name) {}; };

    /** @return string field name in the DB */
    [[nodiscard]] inline const char* GetName() const { return mName; }

    /** @return int number of times modified */
    [[nodiscard]] inline int GetDelta() const { return mDelta; }

    /** @return bool true if was modified from DB */
    [[nodiscard]] inline bool isModified() const { return mDelta > 0; }

    /** Initializes the field's value from the DB */
    virtual void InitDBValue(const MixedValue& value) = 0;

    /** Returns the field's database input value */
    [[nodiscard]] virtual MixedValue GetDBValue() const = 0;

    /** Returns true if the value is a relative increment, not absolute */
    [[nodiscard]] virtual bool UseDBIncrement() const { return false; }
    
    /** Resets this field's delta */
    inline void SetUnmodified() { mDelta = 0; }

protected:

    BaseField(const char* name, BaseObject& parent, int delta = 0);

    /** notifies the database our parent is modified */
    void notifyModified();

    /** name of the field/column in the DB */
    const char* mName;

    /** number of times the field is modified */
    int mDelta;

    /** parent object reference */
    BaseObject& mParent;

    /** database reference */
    ObjectDatabase& mDatabase;
};

// TODO !! add other types - JSON array, ObjectRef
// will definitely want JSON/ObjectRef in other files... JSON will include nlohmann
// and ObjectRef will need to include ObjectDatabase.hpp

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

#endif // LIBA2_FIELDTYPES_H_
