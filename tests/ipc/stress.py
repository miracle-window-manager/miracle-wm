'''
This script stress tests miracle by opening a number of instances of
foot and then performing ipc commands on them until the test is quit.

Do NOT run this in CI.
'''

from i3ipc import Connection
import os
import subprocess
from conftest import Server
import time

def main():
    num_terminals: int = 100
    server = Server(os.environ["SWAYSOCK"], os.environ["WAYLAND_DISPLAY"])
    conn = Connection(server.ipc)
    for i in range(0, num_terminals):
        p = server.open_app("foot")

        if i % 2 != 0:
            conn.command("layout toggle split")

        if i % 10 == 0:
            conn.command(f"workspace {i / 10 + 1}")
        time.sleep(0.5)

    max_workspaces = num_terminals / 10 + 1
    i: int = 1
    while True:
        conn.command(f"workspace {i}")
        i += 1;
        if i == max_workspaces:
            i = 1
        time.sleep(1)


# Using the special variable 
# __name__
if __name__=="__main__":
    main()
