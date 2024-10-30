from i3ipc import Connection
from time import sleep

class TestLayoutCommand:
    def test_splitv(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")
        sleep(1)
        conn.command("layout splitv")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.layout == "splitv"

    def test_toggle_split(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")
        sleep(1)
        conn.command("layout toggle split")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.layout == "splitv"

    def test_tabbed(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")
        sleep(1)
        conn.command("layout tabbed")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.layout == "tabbed"

    def test_stacking(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")
        sleep(1)
        conn.command("layout stacking")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.layout == "stacking"

    def test_toggle_layout_list(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("layout toggle stacking splith tabbed")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.layout == "tabbed"

    def test_toggle_layout_list_nout_found(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("layout toggle stacking tabbed")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.layout == "stacking"