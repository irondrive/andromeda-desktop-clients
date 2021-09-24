
#ifndef LIBA2_ROOTFOLDER_H_
#define LIBA2_ROOTFOLDER_H_

#include <memory>
#include <string>

#include "../Folder.hpp"

class Backend;

/** A root folder that has no parent */
class RootFolder : public Folder
{
public:

	virtual ~RootFolder(){};
    
    /**
     * Load a folder from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     */
    static std::unique_ptr<RootFolder> LoadByID(Backend& backend, const std::string& id);
    
    /** 
     * Construct a folder with JSON data
     * @param backend reference to backend
     * @param data json data from backend with items
     */
    RootFolder(Backend& backend, const nlohmann::json& data);

protected:

    RootFolder(Backend& backend);

private:

    Debug debug;
};

#endif
