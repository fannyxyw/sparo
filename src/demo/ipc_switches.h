

#ifndef DEMO_IPC_SWITCHES_H_
#define DEMO_IPC_SWITCHES_H_

namespace switches {
// The integer value of a file descriptor inherited by the mojo_proxy process
// when launched by its host. This descriptor references a Unix socket which is
// connected to the legacy client application to be the target of this proxy.
// Required.
const char kLegacyClientFd[] = "legacy-client-fd";

// The integer value of a file descriptor inherited by the mojo_proxy process
// when launched by its host. This descriptor references a Unix socket which is
// connected to the host process which launched this proxy to sit between the
// host and some legacy client application.
// Required.
const char kHostIpczTransportFd[] = "host-ipcz-transport-fd";
}

#endif  // DEMO_IPC_SWITCHES_H_
