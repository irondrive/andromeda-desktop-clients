#ifndef LIBA2_PLAINFOLDER_H_
#define LIBA2_PLAINFOLDER_H_

#include <string>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "../Folder.hpp"
#include "Utilities.hpp"

class Backend;

/** A regular Andromeda folder */
class PlainFolder : public Folder
{
public:

    /** Exception indicating that filesystems cannot be deleted  */
    class ModifyException : public Exception { public:
        ModifyException() : Exception("Cannot modify filesystems") {}; };

	virtual ~PlainFolder(){};

    /**
     * Load from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     */
    static std::unique_ptr<PlainFolder> LoadByID(Backend& backend, const std::string& id);
    
    /** 
     * Construct with JSON data and no parent
     * @param backend reference to backend
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     */
    PlainFolder(Backend& backend, const nlohmann::json& data, bool haveItems = false);

    /** 
     * Construct with JSON data
     * @param backend reference to backend 
     * @param parent pointer to parent
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     */
    PlainFolder(Backend& backend, Folder& parent, const nlohmann::json& data, bool haveItems = false);

    virtual void Delete(bool real = false) override;

protected:

    /** 
     * Construct without initializing
     * @param backend reference to backend
     */
    PlainFolder(Backend& backend);

    /** populate itemMap from the backend */
    virtual void LoadItems();

    virtual void SubCreateFile(const std::string& name);

    virtual void SubCreateFolder(const std::string& name);

    virtual void SubRemoveItem(Item& item);

private:

    Debug debug;
};

#endif
