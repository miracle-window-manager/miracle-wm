from i3ipc import Connection
from time import sleep

class TestMoveCommand:
    def test_move_to_workspace_by_number(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")

        sleep(1)
        conn.command("move window to workspace 2")
        sleep(1)
        
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 2
        assert workspace.nodes[0].type == "con"

    def test_move_to_workspace_next(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 2")
        p2 = server.open_app("gnome-calculator")
        sleep(1)

        conn.command("move window to workspace next")
        sleep(1)
        
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 1
        assert len(workspace.nodes) == 2

    def test_move_to_workspace_prev(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 2")
        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("move window to workspace prev")
        
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 1
        assert len(workspace.nodes) == 2

    def test_move_to_workspace_back_and_forth(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 2")
        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("move window to workspace back_and_forth")
        
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 1
        assert len(workspace.nodes) == 2