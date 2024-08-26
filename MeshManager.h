#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "painlessMesh.h"

// Function prototypes for user-defined callbacks
typedef void (*CallbackFunction)(); 

// Class to manage the mesh network
class MeshManager {
public:
    MeshManager(); // Constructor

    void setup(); // Setup function
    void loop();  // Loop function

    // Function to send a message on demand
    void sendMeshMessage(const String &message);

    // Setters for callback functions
    void setWatschnCallback(CallbackFunction func);
    void setSchellnCallback(CallbackFunction func);

private:
    Scheduler userScheduler;
    painlessMesh mesh;

    // Function prototypes
    void sendMessage();
    void watschn();
    void schelln();

    // Callback functions
    static void receivedCallback(uint32_t from, String &msg);
    static void newConnectionCallback(uint32_t nodeId);
    static void changedConnectionCallback();
    static void nodeTimeAdjustedCallback(int32_t offset);

    // Pointers to user-defined callback functions
    static CallbackFunction watschnCallback;
    static CallbackFunction schellnCallback;

    // Static pointer to the MeshManager instance
    static MeshManager* instance;
};

#endif // MESH_MANAGER_H
