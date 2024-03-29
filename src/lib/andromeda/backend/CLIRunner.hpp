#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include <chrono>
#include <list>
#include <string>

#include "BaseRunner.hpp"
#include "RunnerOptions.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"

namespace reproc { class process; }

namespace Andromeda {
namespace Backend {

struct RunnerInput;

/** Runs the API locally by invoking it as a process */
class CLIRunner : public BaseRunner
{
public:

    /** Exception indicating the CLI runner had an error */
    class Exception : public EndpointException { public:
        /** @param msg error message string */
        explicit Exception(const std::string& msg) : 
            EndpointException("Subprocess Error: "+msg) {} 
    };

    /** 
     * @param apiPath path to the API index.php 
     * @param timeout the timeout for each CLI call
     */
    explicit CLIRunner(const std::string& apiPath, const RunnerOptions& runnerOptions);

    [[nodiscard]] std::unique_ptr<BaseRunner> Clone() const override;

    [[nodiscard]] std::string GetHostname() const override { return "local-cli"; }

    using ArgList = std::list<std::string>;
    /** 
     * Runs a system command with the given args, returns the exit code
     * @throws CmdException if there an error running the command
     */
    //static int RunCommand(const ArgList& args);

    std::string RunAction_Read(const RunnerInput& input) override { return RunAction_Write(input); }

    std::string RunAction_Write(const RunnerInput& input) override;

    std::string RunAction_FilesIn(const RunnerInput_FilesIn& input) override;
    
    std::string RunAction_StreamIn(const RunnerInput_StreamIn& input) override;
    
    void RunAction_StreamOut(const RunnerInput_StreamOut& input) override;

    [[nodiscard]] bool RequiresSession() const override { return false; }

private:

    /** Makes sure the given path ends with andromeda-server */
    std::string FixApiPath(std::string apiPath);

    /** @throws Exception if given an error code */
    static void CheckError(reproc::process& process, const std::error_code& error);

    /** Return a list of arguments to run a command with the given input */
    ArgList GetArguments(const RunnerInput& input);

    using EnvList = std::map<std::string, std::string>;
    /** Return a list of environment vars to run a command with the given input */
    EnvList GetEnvironment(const RunnerInput& input);

    /** Prints the argument list if backend debug is enabled */
    void PrintArgs(const ArgList& argList);

    /** Starts the process with the given arguments and environment */
    static void StartProc(reproc::process& process, const ArgList& args, const EnvList& env);

    /** Drains output from the process into the given string (with custom buffer size) */
    static void DrainProc(reproc::process& process, std::string& output, size_t bufferSize);

    /** Waits for the given process to end and returns its exit code */
    static int FinishProc(reproc::process& process, const std::chrono::milliseconds& timeout);

    mutable Debug mDebug;

    const std::string mApiPath;
    const RunnerOptions mOptions;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_CLIRUNNER_H_
