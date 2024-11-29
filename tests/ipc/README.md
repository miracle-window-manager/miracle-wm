# IPC Testing
This subproject intends to test IPC interactions with miracle-wm's socket. This
is accomplished by starting a headless miracle server and sending IPC requests
to it. The goal is to have complete coverage for miracle-wm's IPC.

## Installation
```sh
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Running
```sh
source venv/bin/activate
pytest
```

### Environment variables
- `MIRACLE_IPC_TEST_USE_ENV`: If set to true, tests will use the local environment to test
  miracle-wm against instead of spawning miracle-wm itself.
- `MIRACLE_IPC_TEST_BIN`: can be set to a `path/to/miracle-wm`. Defaults to `miracle-wm`.