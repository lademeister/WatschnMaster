#include "MeshManager.h"

#define   MESH_PREFIX     "ESPMesh_WatschnFM-Remote"
#define   MESH_PASSWORD   "gyntroff"
#define   MESH_PORT       5555

// Initialize static member variables
CallbackFunction MeshManager::watschnCallback = nullptr;
CallbackFunction MeshManager::schellnCallback = nullptr;
MeshManager* MeshManager::instance = nullptr;

// Static member function implementations
void MeshManager::receivedCallback(uint32_t from, String &msg) {
    if (instance) {
        Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

        // Call functions based on message content
        if (msg == "watschn" && watschnCallback) {
            watschnCallback();
        } else if (msg == "schelln" && schellnCallback) {
            schellnCallback();
        }
    }
}

void MeshManager::newConnectionCallback(uint32_t nodeId) {
    if (instance) {
        Serial.printf("--> New Connection, nodeId = %u\n", nodeId);
    }
}

void MeshManager::changedConnectionCallback() {
    if (instance) {
        Serial.printf("Changed connections\n");
    }
}

void MeshManager::nodeTimeAdjustedCallback(int32_t offset) {
    if (instance) {
        Serial.printf("Adjusted time %u. Offset = %d\n", instance->mesh.getNodeTime(), offset);
    }
}

// Constructor
MeshManager::MeshManager() {
    instance = this; // Set the static instance pointer to this instance
}

// Setup function
void MeshManager::setup() {
    //Serial.begin(115200);

    mesh.setDebugMsgTypes(ERROR | STARTUP);  // Set before init() so that you can see startup messages

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);   
    mesh.onReceive(receivedCallback);
    mesh.onNewConnection(newConnectionCallback);
    mesh.onChangedConnections(changedConnectionCallback);
    mesh.onNodeTimeAdjusted(nodeTimeAdjustedCallback);
}

// Loop function
void MeshManager::loop() {
    // It will run the user scheduler as well
    mesh.update();
}

// Function to send a message on demand
void MeshManager::sendMeshMessage(const String &message) {
    Serial.printf("Sending message: %s\n", message.c_str());
    mesh.sendBroadcast(message);
}

// Setters for callback functions
void MeshManager::setWatschnCallback(CallbackFunction func) {
    watschnCallback = func;
}

void MeshManager::setSchellnCallback(CallbackFunction func) {
    schellnCallback = func;
}
