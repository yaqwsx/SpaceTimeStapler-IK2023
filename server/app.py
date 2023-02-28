from fastapi import FastAPI
from pydantic import BaseModel, PrivateAttr
from typing import Dict
from time import time
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class ButtonState(BaseModel):
    lastPress: float = 0 # timestamp
    lastActive: float = 0 # timestamp

class ButtonsGameSettings(BaseModel):
    pressTolerance: float = 5 # seconds
    activityTolerance: float = 10 # seconds
    revealDuration: float = 30 # seconds
    message: str = "Vložte tajnou zprávu"

class ButtonsGame(BaseModel):
    buttons: Dict[int, ButtonState] = {}
    _buttonIdCounter: int = PrivateAttr(default=0)

    hideAt: float = 0 # timestamp

    settings: ButtonsGameSettings = ButtonsGameSettings()


    @property
    def active(self) -> None:
        return self.hideAt > time()

    def onButtonPress(self, id: int) -> None:
        timestamp = time()

        self.buttons.setdefault(id, ButtonState()).lastPress = timestamp

        self.purgeButtons(timestamp)
        press = [x.lastPress for x in self.buttons.values()]
        if max(press) - min(press) < self.settings.pressTolerance:
            self.hideAt = timestamp + self.settings.revealDuration

    def onButtonActivity(self, id: int) -> None:
        timestamp = time()

        self.buttons.setdefault(id, ButtonState()).lastActive = timestamp
        self.purgeButtons(timestamp)

    def getNewButtonId(self) -> int:
        self._buttonIdCounter += 1
        return self._buttonIdCounter

    def purgeButtons(self, timestamp: float) -> None:
        threshold = timestamp - self.settings.activityTolerance
        self.buttons = {i: b for i, b in self.buttons.items() if b.lastActive > threshold}

class BigGame(BaseModel):
    roundStart: float = 0 # timestamp
    restarting: bool = False

    @property
    def running(self):
        return self.roundStart != 0

    def start(self):
        self.roundStart = time()
        self.restarting = False

    def stop(self):
        self.roundStart = 0
        self.restarting = False

    def restart(self):
        self.roundStart = 0
        self.restarting = True

class LaserGame(BaseModel):
    lastActive: float = 0 # timestamp

class LanternState(BaseModel):
    lastActive: float = 0 # timestamp

class LanternGame(BaseModel):
    window1start: float = 0
    window1duration: float = 0
    window2start: float = 0
    window2duration: float = 0
    window3start: float = 0
    window3duration: float = 0
    lanterns: Dict[int, LanternState] = {}

    _lanternIdCounter: int = PrivateAttr(default=0)

    def getNewLanternId(self) -> int:
        self._lanternIdCounter += 1
        return self._lanternIdCounter

    def onLanternActivity(self, id: int) -> None:
        timestamp = time()

        self.lanterns.setdefault(id, LanternState()).lastActive = timestamp
        self.purgeLanterns(timestamp)

    def purgeLanterns(self, timestamp: float) -> None:
        threshold = timestamp - 20
        self.lanterns = {i: b for i, b in self.lanterns.items() if b.lastActive > threshold}


class ChainGame(BaseModel):
    activeTime: float = 0
    active: bool = False


BUTTONS_GAME = ButtonsGame()
BIG_GAME = BigGame()
LASER_GAME = LaserGame()
LANTERN_GAME = LanternGame()
CHAIN_GAME = ChainGame()


# Buttons ----------------------------------------------------------------------

@app.get("/buttons/register")
def buttonRegister():
    return {
        "id": BUTTONS_GAME.getNewButtonId()
    }

@app.post("/buttons/{id}/register")
def buttonRegister(id: int):
    BUTTONS_GAME.onButtonActivity(id)
    return {}

@app.post("/buttons/{id}/press")
def buttonPress(id: int):
    BUTTONS_GAME.onButtonPress(id)
    return {}

@app.get("/buttons/settings")
def buttonSettings():
    return BUTTONS_GAME.settings

@app.post("/buttons/settings")
def buttonSettings(settings: ButtonsGameSettings):
    BUTTONS_GAME.settings = settings
    return BUTTONS_GAME.settings

@app.get("/buttons/decision")
def buttonDecision():
    return {
        "revealed": BUTTONS_GAME.active,
        "pressTolerance": BUTTONS_GAME.settings.pressTolerance
    }

@app.get("/buttons/state")
def buttonState():
    return {
        "revealed": BUTTONS_GAME.active,
        "message": BUTTONS_GAME.settings.message if BUTTONS_GAME.active else None,
        "buttons": BUTTONS_GAME.buttons,
        "time": time()
    }

# Big game ---------------------------------------------------------------------

@app.get("/state")
def gameState():
    return {
        "running": BIG_GAME.running,
        "start": BIG_GAME.roundStart,
        "time": time(),
        "restarting": BIG_GAME.restarting
    }

@app.post("/start")
def start():
    BIG_GAME.start()
    return {}

@app.post("/stop")
def start():
    BIG_GAME.stop()
    return {}

@app.post("/restart")
def start():
    BIG_GAME.restart()
    return {}


# Lantern game -----------------------------------------------------------------

@app.get("/lanterns/register")
def lanternRegister():
    return {
        "id": LANTERN_GAME.getNewLanternId()
    }

@app.post("/lanterns/{id}/register")
def lanternRegister(id: int):
    LANTERN_GAME.onLanternActivity(id)
    return {}

@app.get("/lanterns/state")
def lanternState():
    return {
        "lanterns": LANTERN_GAME.lanterns,
        "time": time()
    }

@app.get("/lanterns/doors")
def lanternDoors():
    def response(a, b, c, d):
        return {f"door{key}": val for key, val in zip("ABCD", [a, b, c, d])}

    if BIG_GAME.restarting:
        return response(True, True, True, True)
    if not BIG_GAME.running:
        return response(False, False, False, False)

    roundTime = time() - BIG_GAME.roundStart

    return {
        "doorA": CHAIN_GAME.active,
        "doorB": LANTERN_GAME.window1start < roundTime < (LANTERN_GAME.window1start + LANTERN_GAME.window1duration),
        "doorC": LANTERN_GAME.window2start < roundTime < (LANTERN_GAME.window2start + LANTERN_GAME.window2duration),
        "doorD": LANTERN_GAME.window3start < roundTime < (LANTERN_GAME.window3start + LANTERN_GAME.window3duration),
    }
