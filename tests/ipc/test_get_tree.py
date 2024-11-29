from i3ipc import Connection
import os
import subprocess
from time import sleep

class TestGetTree:
    def test_empty_tree(self, server):
        conn = Connection(server.ipc)
        container = conn.get_tree()
        assert container.type == "root"

        output = container.nodes[0]
        assert output.type == "output"

        workspace = output.nodes[0]
        assert workspace.type == "workspace"

        assert len(workspace.nodes) == 0

    def test_regular_container(self, server):
        # Open gedit first
        my_env = os.environ.copy()
        my_env['WAYLAND_DISPLAY'] = server.wayland
        p = subprocess.Popen(['gedit'], env=my_env)
        sleep(1)  # Give gedit some time to settle down and open

        conn = Connection(server.ipc)
        container = conn.get_tree()
        output = container.nodes[0]
        workspace = output.nodes[0]
        
        assert len(workspace.nodes) == 1
        app = workspace.nodes[0]
        assert app.focused == True
        assert app.percent == 1.0

    def test_percent_two_containers(self, server):
        # Open gedit first
        my_env = os.environ.copy()
        my_env['WAYLAND_DISPLAY'] = server.wayland
        p1 = subprocess.Popen(['gedit'], env=my_env)
        sleep(1)  # Give gedit some time to settle down and open
        p2 = subprocess.Popen(['gnome-chess'], env=my_env)
        sleep(1)  # Give gedit some time to settle down and open

        conn = Connection(server.ipc)
        container = conn.get_tree()
        output = container.nodes[0]
        workspace = output.nodes[0]
        
        assert len(workspace.nodes) == 2
        app = workspace.nodes[1]
        assert app.focused == True
        assert app.percent == 0.5

    def test_two_outputs(self, multi_win_server):
        conn = Connection(multi_win_server.ipc)
        container = conn.get_tree()
        assert len(container.nodes) == 2
