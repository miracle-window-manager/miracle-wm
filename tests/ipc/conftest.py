import pytest
import subprocess
from subprocess import Popen, PIPE, STDOUT
import os
from typing import List, Tuple, Dict

class Server:
    def __init__(self, ipc: str, wayland: str) -> None:
        self.ipc = ipc
        self.wayland = wayland

    def open_app(self, command: str):
        my_env = os.environ.copy()
        my_env['WAYLAND_DISPLAY'] = self.wayland
        return subprocess.Popen([command], env=my_env)
    
def _create_server(args: List[str]) -> Tuple[Popen[bytes], Dict[str, str]]:
    command = "miracle-wm"
    if "MIRACLE_IPC_TEST_BIN" in os.environ:
        command = os.environ["MIRACLE_IPC_TEST_BIN"]

    env = os.environ.copy()
    env['WAYLAND_DISPLAY'] = 'wayland-98'
    process = Popen([command] + args,
                    env=env, stdout=PIPE, stderr=STDOUT)    
    return (process, env)

@pytest.fixture(scope="function")
def server():
    if "MIRACLE_IPC_TEST_USE_ENV" in os.environ:
        yield Server(os.environ["SWAYSOCK"], os.environ["WAYLAND_DISPLAY"])
        return
    
    (process, env) = _create_server(['--platform-display-libs', 'mir:virtual', '--virtual-output', '800x600', '--no-config', '1'])
    socket = ""
    to_find = "Listening to IPC socket on path: "   
    with process.stdout:
        for line in iter(process.stdout.readline, b''):
            data = line.decode("utf-8").strip()
            # print(data)
            if to_find in data:
                i = data.index(to_find)
                i = i + len(to_find)
                socket = data[i:].strip()
                break

        yield Server(socket, env["WAYLAND_DISPLAY"])

        # for line in iter(process.stdout.readline, b''):
        #     data = line.decode("utf-8").strip()
        #     print(data)

        process.terminate()
        return
    

@pytest.fixture(scope="function")
def multi_win_server():
    if "MIRACLE_IPC_TEST_USE_ENV" in os.environ:
        yield Server(os.environ["SWAYSOCK"], os.environ["WAYLAND_DISPLAY"])
        return
    
    (process, env) = _create_server(['--platform-display-libs', 'mir:virtual', '--virtual-output', '800x600', '--virtual-output', '400x300', '--no-config', '1'])
    socket = ""
    to_find = "Listening to IPC socket on path: "   
    with process.stdout:
        for line in iter(process.stdout.readline, b''):
            data = line.decode("utf-8").strip()
            # print(data)
            if to_find in data:
                i = data.index(to_find)
                i = i + len(to_find)
                socket = data[i:].strip()
                break

        yield Server(socket, env["WAYLAND_DISPLAY"])

        # for line in iter(process.stdout.readline, b''):
        #     data = line.decode("utf-8").strip()
        #     print(data)

        process.terminate()
        return