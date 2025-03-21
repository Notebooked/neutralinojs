#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <vector>

#include "extensions_loader.h"
#include "settings.h"
#include "helpers.h"
#include "auth/authbasic.h"
#include "api/os/os.h"
#include "api/debug/debug.h"

using namespace std;
using json = nlohmann::json;

namespace extensions {

vector<string> loadedExtensions;
bool initialized = false;

json __buildExtensionProcessInput(const string &extensionId) {
    json options = {
        {"nlPort", to_string(settings::getOptionForCurrentMode("port").get<int>())},
        {"nlToken", authbasic::getTokenInternal()},
        {"nlConnectToken", authbasic::getConnectTokenInternal()},
        {"nlExtensionId", extensionId}
    };
    return options;
}

void init() {
    json jExtensions = settings::getOptionForCurrentMode("extensions");
    if(jExtensions.is_null())
        return;
    vector<json> extensions = jExtensions.get<vector<json>>();
    for(const json &extension: extensions) {
        string commandKeyForOs = "command" + string(NEU_OS_NAME);

        if(!helpers::hasField(extension, "id")) {
            continue;
        }

        string extensionId = extension["id"].get<string>();

        if(helpers::hasField(extension, "command") || helpers::hasField(extension, commandKeyForOs)) {
            string command = helpers::hasField(extension, commandKeyForOs) ? extension[commandKeyForOs].get<string>()
                                : extension["command"].get<string>();
            command = regex_replace(command, regex("\\$\\{NL_PATH\\}"), settings::getAppPath());
            
            pair<int, int> res = os::spawnProcess(command,
                [=](int pid, string stdOut) { //stdOut handler
                    debug::log(debug::LogTypeInfo, "[" + extensionId + "]: " + stdOut);
                }, [=](int pid, string stdErr) { //stdErr handler
                    debug::log(debug::LogTypeError, "[" + extensionId + "]: " + stdErr);
                }, [=](int pid) { //exit handler
                    debug::log(debug::LogTypeInfo, "[" + extensionId + "]: Process exited.");
                });
            int id = res.first;
            os::updateSpawnedProcess(os::SpawnedProcessEvent{ id, "stdIn", __buildExtensionProcessInput(extensionId).dump() });
            os::updateSpawnedProcess(os::SpawnedProcessEvent{ id, "stdInEnd" });
        }

        extensions::loadOne(extensionId);
    }
    initialized = true;
}

void loadOne(const string &extensionId) {
    loadedExtensions.push_back(extensionId);
}

vector<string> getLoaded() {
    return loadedExtensions;
}

bool isLoaded(const string &extensionId) {
    return find(loadedExtensions.begin(), loadedExtensions.end(), extensionId)
            != loadedExtensions.end();
}

bool isInitialized() {
    return initialized;
}

} // namespace extensions
