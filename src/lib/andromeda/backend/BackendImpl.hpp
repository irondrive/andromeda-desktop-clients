#ifndef LIBA2_BACKENDIMPL_H_
#define LIBA2_BACKENDIMPL_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include "nlohmann/json_fwd.hpp"

#include "BackendException.hpp"
#include "Config.hpp"
#include "RunnerInput.hpp"
#include "andromeda/common.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace Account { class Session; }

namespace Backend {
class RunnerPool;

/** 
 * Manages communication with the backend API 
 * THREAD SAFE (INTERNAL LOCKS) - except Authentication
 */
class BackendImpl
{
public:

    /** Exception indicating there was a JSON format error */
    class JSONErrorException : public BackendException { public:
        /** @param error the JSON error message */
        explicit JSONErrorException(const std::string& error) : 
            BackendException("JSON Error: "+error) {}; };

    /** Exception indicating that the backend did not return the correct # of bytes */
    class ReadSizeException : public BackendException { public:
        /** @param wanted number of bytes expected
         * @param got number of bytes received */
        explicit ReadSizeException(size_t wanted, size_t got) : BackendException(
            "Wanted "+std::to_string(wanted)+" bytes, got "+std::to_string(got)) {}; };

    /** Exception indicating we set the backend as read-only */
    class ReadOnlyException : public BackendException { public:
        explicit ReadOnlyException() : BackendException("Read Only Backend") {}; };

    /** Exception indicating the requested write is too large to send */
    class WriteSizeException : public BackendException { public:
        explicit WriteSizeException() : BackendException("Write Size Too Large") {}; };

    /** Base exception for Andromeda-returned errors */
    class APIException : public BackendException { public:
        using BackendException::BackendException; // string constructor
        APIException(int code, const std::string& message) : 
            BackendException("API code:"+std::to_string(code)+" message:"+message) {}; };

    /** Andromeda exception indicating the requested operation is invalid */
    class UnsupportedException : public APIException { public:
        UnsupportedException() : APIException("Invalid Operation") {}; };

    /** Base exception for Andromeda-returned 403 errors */
    class DeniedException : public APIException { public:
        DeniedException() : APIException("Access Denied") {};
        /** @param message message from backend */
        explicit DeniedException(const std::string& message) : APIException(message) {}; };

    /** Andromeda exception indicating authentication failed */
    class AuthenticationFailedException : public DeniedException { public:
        AuthenticationFailedException() : DeniedException("Authentication Failed") {}; };

    /** Andromeda exception indicating the session in use is invalid */
    class InvalidSessionException : public DeniedException { public:
        InvalidSessionException() : DeniedException("Invalid Session") {}; };

    /** Andromeda exception indicating two factor is needed */
    class TwoFactorRequiredException : public DeniedException { public:
        TwoFactorRequiredException() : DeniedException("Two Factor Required") {}; };

    /** Andromeda exception indicating the server or FS are read only */
    class ReadOnlyFSException : public DeniedException { public:
        /** @param which string describing what is read-only */
        explicit ReadOnlyFSException(const std::string& which) : DeniedException("Read Only "+which) {}; };

    /** Base exception for Andromeda-returned 404 errors */
    class NotFoundException : public APIException { public:
        NotFoundException() : APIException("Not Found") {};
        /** @param message message from backend */
        explicit NotFoundException(const std::string& message) : APIException(message) {}; };

    /**
     * @param options configuration options
     * @param runners the RunnerPool to use for requests
     * @throws BackendException for backend issues
     */
    BackendImpl(const Andromeda::ConfigOptions& options, RunnerPool& runners);

    virtual ~BackendImpl();
    DELETE_COPY(BackendImpl)
    DELETE_MOVE(BackendImpl)

    /** Gets the server config object */
    inline const Config& GetConfig() { return mConfig; }

    /** Returns the backend options in use */
    [[nodiscard]] inline const Andromeda::ConfigOptions& GetOptions() const { return mOptions; }

    /** Returns true if doing memory only */
    [[nodiscard]] bool isMemory() const;

    /** Returns true if the backend is read-only */
    [[nodiscard]] bool isReadOnly() const;

    /** Returns true if this backend requires using a session */
    bool RequiresSession() const;
    /** Sets the session to use (or nullptr if none) */
    void SetSession(Account::Session* session);
    /** Sets the username to masquerade as (or "" if none) */
    void SetSudoUsername(const std::string& username){ mSudoUsername = username; }

    /** Returns true if the backend is using an account with the server */
    bool UsingAccount() const { return RequiresSession() ? (mSession != nullptr) : (!mSudoUsername.empty()); }

    /*****************************************************/
    // ---- Actual backend functions below here ---- //

    /**
     * Loads all server app config
     * @return loaded config as JSON
     * @throws BackendException for backend issues
     */
    nlohmann::json GetAppConfigJ();

    /** 
     * Load files policy for the current account
     * @throws BackendException for backend issues
     */
    nlohmann::json GetFilesPolicy();

    /**
     * Load account metadata for the current account
     * @param session use this session rather than mSession if given
     * @return account metadata as JSON
     * @throws BackendException for backend issues
     */
    nlohmann::json GetAccount(const Account::Session* session = nullptr);

    /**
     * Creates a new session with the backend
     * @param username username to log in with
     * @param passkeyb64 passkey as base64
     * @param twofactor two factor code
     * @return session metadata as JSON
     * @throws AuthenticationFailedException in particular for wrong username/password
     * @throws TwoFactorRequiredException if two factor is required and not given
     * @throws BackendException for any other backend issues
     */
    nlohmann::json CreateSession(const std::string& username, const std::string& passkeyb64, const std::string& twofactor = "");

    /**
     * Deletes the current client (from SetSession) from the backend
     * @param session use this session rather than mSession if given
     */
    void DeleteClient(const Account::Session* session = nullptr);

    /** Returns the password salt to use for a username */
    std::string GetPasswordSalt(const std::string& username);

    /**
     * Load folder metadata (with subitems)
     * @param id folder ID (or blank for default)
     * @throws BackendException for backend issues
     */
    nlohmann::json GetFolder(const std::string& id = "");

    /**
     * Load root folder metadata (no subitems)
     * @param id filesystem ID (or blank for default)
     * @throws BackendException for backend issues
     */
    nlohmann::json GetRootFolder(const std::string& id = "");

    /**
     * Load filesystem metadata
     * @param id filesystem ID (or blank for default)
     * @throws BackendException for backend issues
     */
    nlohmann::json GetStorage(const std::string& id = "");

    /**
     * Load policy for a filesystem
     * @param id filesystem ID
     * @throws BackendException for backend issues
     */
    nlohmann::json GetStoragePolicy(const std::string& id);

    /** 
     * Loads filesystem list metadata
     * @throws BackendException for backend issues
     */
    nlohmann::json GetStorages();

    /** 
     * Loads items owned but in another user's parent
     * @throws BackendException for backend issues 
     */
    nlohmann::json GetAdopted();
    
    /**
     * Creates a new file
     * @param parent parent folder ID
     * @param name name of new file
     * @param overwrite whether to overwrite existing
     * @throws BackendException for backend issues
     */
    nlohmann::json CreateFile(const std::string& parent, const std::string& name, bool overwrite = false);

    /**
     * Creates a new folder
     * @param parent parent folder ID
     * @param name name of new folder
     * @throws BackendException for backend issues
     */
    nlohmann::json CreateFolder(const std::string& parent, const std::string& name);

    /**
     * Deletes a file by ID
     * @throws BackendException for backend issues
     */
    void DeleteFile(const std::string& id);

    /** 
     * Deletes a folder by ID
     * @throws BackendException for backend issues
     */
    void DeleteFolder(const std::string& id);

    /** 
     * Renames a file
     * @param id file ID
     * @param name new name
     * @param overwrite whether to overwrite existing
     * @throws BackendException for backend issues
     */
    nlohmann::json RenameFile(const std::string& id, const std::string& name, bool overwrite = false);

    /** 
     * Renames a folder
     * @param id folder ID
     * @param name new name
     * @param overwrite whether to overwrite existing
     * @throws BackendException for backend issues
     */
    nlohmann::json RenameFolder(const std::string& id, const std::string& name, bool overwrite = false);

    /** 
     * Moves a file
     * @param id file ID
     * @param parent new parent ID
     * @param overwrite whether to overwrite existing
     * @throws BackendException for backend issues
     */
    nlohmann::json MoveFile(const std::string& id, const std::string& parent, bool overwrite = false);

    /** 
     * Moves a file
     * @param id file ID
     * @param parent new parent ID
     * @param overwrite whether to overwrite existing
     * @throws BackendException for backend issues
     */
    nlohmann::json MoveFolder(const std::string& id, const std::string& parent, bool overwrite = false);

    /**
     * Reads data from a file
     * @param id file ID
     * @param offset offset to read from
     * @param length number of bytes to read
     * @throws BackendException for backend issues
     */
    std::string ReadFile(const std::string& id, uint64_t offset, size_t length);

    /**
     * Streams data from a file
     * @param id file ID
     * @param offset offset to read from
     * @param length number of bytes to read
     * @param userFunc data handler function
     * @throws BackendException for backend issues
     */
    void ReadFile(const std::string& id, uint64_t offset, size_t length, const ReadFunc& userFunc);

    /**
     * Writes data to a file
     * @param id file ID
     * @param offset offset to write to
     * @param data file data to write
     * @throws BackendException for backend issues
     */
    nlohmann::json WriteFile(const std::string& id, uint64_t offset, const std::string& data);
    
    /**
     * Writes data to a file (streaming)
     * @param id file ID
     * @param offset offset to write to
     * @param userFunc function to stream data
     * @throws BackendException for backend issues
     */
    nlohmann::json WriteFile(const std::string& id, uint64_t offset, const WriteFunc& userFunc);
    
    /**
     * Creates a new file with data
     * @param parent parent folder ID
     * @param name name of new file
     * @param data file data to write
     * @param oneshot if true, can't split into multiple writes
     * @param overwrite whether to overwrite existing
     * @throws WriteSizeException if oneshot is true and too big for one upload
     * @throws BackendException for backend issues
     */
    nlohmann::json UploadFile(const std::string& parent, const std::string& name, const std::string& data, 
        bool oneshot = false, bool overwrite = false);

    /**
     * Creates a new file with data (streaming)
     * @param parent parent folder ID
     * @param name name of new file
     * @param userFunc function to stream data
     * @param oneshot if true, can't split into multiple writes
     * @param overwrite whether to overwrite existing
     * @throws WriteSizeException if oneshot is true and too big for one upload
     * @throws BackendException for backend issues
     */
    nlohmann::json UploadFile(const std::string& parent, const std::string& name, const WriteFunc& userFunc, 
        bool oneshot = false, bool overwrite = false);

    /**
     * Truncates a file
     * @param id file ID
     * @param size new file size
     * @throws BackendException for backend issues
     */
    nlohmann::json TruncateFile(const std::string& id, uint64_t size);

private:
    
    /** 
     * Augment input with session authentication details
     * @param session if given, use this instead of mSession
     */
    template <class InputT>
    InputT& FinalizeInput(InputT& input, const Account::Session* session = nullptr);

    /** Prints a RunnerInput to the given stream */
    static void PrintInput(const RunnerInput& input, std::ostream& str, const std::string& myfname, uint64_t reqCount);
    /** Prints a RunnerInput_FilesIn to the given stream */
    static void PrintInput(const RunnerInput_FilesIn& input, std::ostream& str, const std::string& myfname, uint64_t reqCount);
    /** Prints a RunnerInput_StreamIn to the given stream */
    static void PrintInput(const RunnerInput_StreamIn& input, std::ostream& str, const std::string& myfname, uint64_t reqCount);

    /** Parses and returns standard Andromeda JSON */
    nlohmann::json GetJSON(const std::string& resp);

    /** Finalizes input, runs the action, returns string */
    std::string RunAction_ReadStr(RunnerInput& input, const Account::Session* session = nullptr);
    /** Finalizes input, runs the action, returns JSON */
    nlohmann::json RunAction_Read(RunnerInput& input, const Account::Session* session = nullptr);
    /** Finalizes input, runs the action, returns JSON */
    nlohmann::json RunAction_Write(RunnerInput& input, const Account::Session* session = nullptr);
    /** Finalizes input, runs the action, returns JSON */
    nlohmann::json RunAction_FilesIn(RunnerInput_FilesIn& input);
    /** Finalizes input, runs the action, returns JSON */
    nlohmann::json RunAction_StreamIn(RunnerInput_StreamIn& input);
    /** Finalizes input, runs the action, returns JSON */
    void RunAction_StreamOut(RunnerInput_StreamOut& input);

    /** Function that is given a WriteFunc and returns a RunnerInput_StreamIn for file upload */
    using UploadInput = std::function<RunnerInput_StreamIn (const WriteFunc&)>;

    /**
     * Commonized file upload/write stream with max upload size checking/retries
     * @param userFunc user-provided data streaming function
     * @param id ID of the file if already created (getUpload=nullptr)
     * @param offset offset of the file to write to if already created (getUpload=nullptr)
     * @param getUpload function to get an input for the initial upload if NOT already created (ignore id,offset)
     * @param oneshot if true, can't split into multiple writes
     * @throws WriteSizeException if oneshot is true and too big for one upload
     */
    nlohmann::json SendFile(const WriteFunc& userFunc, std::string id, uint64_t offset, const UploadInput& getUpload, bool oneshot);

    /** Session to use with requests */
    Account::Session* mSession { nullptr };
    /** --auth_sudouser sudo user to use */
    std::string mSudoUsername;

    // global backend request counter for debug
    static std::atomic<uint64_t> sReqNext;

    ConfigOptions mOptions;
    RunnerPool& mRunners;

    mutable Debug mDebug;
    Config mConfig;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_BACKENDIMPL_H_
