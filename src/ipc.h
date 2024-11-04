/**
Copyright (C) 2024  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MIRACLEWM_IPC_H
#define MIRACLEWM_IPC_H

#include "i3_command.h"
#include "i3_command_executor.h"
#include "mode_observer.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <mir/fd.h>
#include <mir/server_action_queue.h>
#include <miral/runner.h>
#include <shared_mutex>
#include <vector>

struct sockaddr_un;

namespace miracle
{

class Policy;
class Config;

/// This it taken directly from SWAY
enum IpcCommandType
{
    // i3 command types - see i3's I3_REPLY_TYPE constants
    IPC_COMMAND = 0,
    IPC_GET_WORKSPACES = 1,
    IPC_SUBSCRIBE = 2,
    IPC_GET_OUTPUTS = 3,
    IPC_GET_TREE = 4,
    IPC_GET_MARKS = 5,
    IPC_GET_BAR_CONFIG = 6,
    IPC_GET_VERSION = 7,
    IPC_GET_BINDING_MODES = 8,
    IPC_GET_CONFIG = 9,
    IPC_SEND_TICK = 10,
    IPC_SYNC = 11,
    IPC_GET_BINDING_STATE = 12,

    // sway-specific command types
    IPC_GET_INPUTS = 100,
    IPC_GET_SEATS = 101,

    // Events sent from sway to clients. Events have the highest bits set.
    IPC_EVENT_WORKSPACE = ((1 << 31) | 0),
    IPC_EVENT_OUTPUT = ((1 << 31) | 1),
    IPC_EVENT_MODE = ((1 << 31) | 2),
    IPC_EVENT_WINDOW = ((1 << 31) | 3),
    IPC_EVENT_BARCONFIG_UPDATE = ((1 << 31) | 4),
    IPC_EVENT_BINDING = ((1 << 31) | 5),
    IPC_EVENT_SHUTDOWN = ((1 << 31) | 6),
    IPC_EVENT_TICK = ((1 << 31) | 7),

    // sway-specific event types
    IPC_EVENT_BAR_STATE_UPDATE = ((1 << 31) | 20),
    IPC_EVENT_INPUT = ((1 << 31) | 21),
};

/// Inter process communication for compositor clients (e.g. waybar).
/// This class will implement I3's interface: https://i3wm.org/docs/ipc.html
/// plus some of the sway-specific items.
/// It may be extended in the future.
class Ipc : public virtual WorkspaceObserver, public virtual ModeObserver
{
public:
    Ipc(miral::MirRunner& runner,
        WorkspaceManager&,
        Policy& policy,
        std::shared_ptr<mir::ServerActionQueue> const&,
        I3CommandExecutor&,
        std::shared_ptr<Config> const&);
    ~Ipc();

    void on_created(uint32_t id) override;
    void on_removed(uint32_t id) override;
    void on_focused(std::optional<uint32_t>, uint32_t) override;
    void on_changed(WindowManagerMode mode) override;
    void on_shutdown();

private:
    struct IpcClient
    {
        mir::Fd client_fd;
        std::unique_ptr<miral::FdHandle> handle;
        uint32_t pending_read_length = 0;
        IpcCommandType pending_type;
        std::vector<char> buffer;
        int write_buffer_len = 0;
        int subscribed_events = 0;
    };

    WorkspaceManager& workspace_manager;
    Policy& policy;
    mir::Fd ipc_socket;
    std::unique_ptr<miral::FdHandle> socket_handle;
    sockaddr_un* ipc_sockaddr = nullptr;
    std::vector<IpcClient> clients;
    std::vector<I3ScopedCommandList> pending_commands;
    mutable std::shared_mutex pending_commands_mutex;
    std::shared_ptr<mir::ServerActionQueue> queue;
    I3CommandExecutor& executor;
    std::shared_ptr<Config> config;

    void disconnect(IpcClient& client);
    IpcClient& get_client(int fd);
    void handle_command(IpcClient& client, uint32_t payload_length, IpcCommandType payload_type);
    void send_reply(IpcClient& client, IpcCommandType command_type, std::string const& payload);
    void handle_writeable(IpcClient& client);
    bool parse_i3_command(std::string_view const& command);
};
}

#endif // MIRACLEWM_IPC_H
