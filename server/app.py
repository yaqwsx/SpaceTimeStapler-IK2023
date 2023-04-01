from fastapi import FastAPI, Request
from pydantic import BaseModel, PrivateAttr
from typing import Dict, List
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
    enabled: bool = False

class LanternState(BaseModel):
    lastActive: float = 0 # timestamp
    battery: float = 0 # percentage

class LanternSettings(BaseModel):
    window1start: float = 60
    window1duration: float = 10
    window2start: float = 300
    window2duration: float = 10
    window3start: float = 450
    window3duration: float = 10

class LanternGame(BaseModel):
    settings: LanternSettings = LanternSettings()
    lanterns: Dict[int, LanternState] = {}

    _lanternIdCounter: int = PrivateAttr(default=0)

    def getNewLanternId(self) -> int:
        self._lanternIdCounter += 1
        return self._lanternIdCounter

    def onLanternActivity(self, id: int, battery: int) -> None:
        timestamp = time()

        self.lanterns.setdefault(id, LanternState()).lastActive = timestamp
        self.lanterns.setdefault(id, LanternState()).battery = battery
        self.purgeLanterns(timestamp)

    def purgeLanterns(self, timestamp: float) -> None:
        threshold = timestamp - 20
        self.lanterns = {i: b for i, b in self.lanterns.items() if b.lastActive > threshold}


class ChainGame(BaseModel):
    lastActive: float = 0 # timestamp
    active: bool = False
    lastValue: int = 0
    override: bool = False

    def tick(self):
        if time() - self.lastActive > 10:
            self.active = False

class CpuSettings(BaseModel):
    dischargeRate: float = 0
    capFuel: float = 0.1
    capThreshold: int = 1000
    overrideFuel: bool = False


class CpuGame(BaseModel):
    lastActive: float = 0 # timestamp
    phase: int = 0
    fuel: float = 0
    lastCap: float = 0
    password: str = ""
    commandQueue: List[str] = None
    settings: CpuSettings = CpuSettings()


BUTTONS_GAME = ButtonsGame()
BIG_GAME = BigGame()
LASER_GAME = LaserGame()
LANTERN_GAME = LanternGame()
CHAIN_GAME = ChainGame()
CPU_GAME = CpuGame()


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

class LanternRegistration(BaseModel):
    battery: int

@app.post("/lanterns/{id}/register")
async def lanternRegister(id: int, body: LanternRegistration):
    LANTERN_GAME.onLanternActivity(id, body.battery)
    return {}

@app.get("/lanterns/state")
def lanternState():
    LANTERN_GAME.purgeLanterns(time())
    return {
        "lanterns": LANTERN_GAME.lanterns,
        "time": time()
    }

@app.get("/lanterns/doors")
def lanternDoors():
    def response(a, b, c, d):
        base = {
            "restarting": BIG_GAME.restarting
        }
        base.update({f"door{key}": val for key, val in zip("ABCD", [a, b, c, d])})
        return base

    if BIG_GAME.restarting:
        return response(True, True, True, True)
    if not BIG_GAME.running:
        return response(False, False, False, False)

    roundTime = time() - BIG_GAME.roundStart

    sett = LANTERN_GAME.settings
    CHAIN_GAME.tick()
    print(CHAIN_GAME)
    return response(
        CHAIN_GAME.active or CHAIN_GAME.override,
        sett.window1start < roundTime < (sett.window1start + sett.window1duration),
        sett.window2start < roundTime < (sett.window2start + sett.window2duration),
        sett.window3start < roundTime < (sett.window3start + sett.window3duration))

@app.get("/lanterns/settings")
def lanternSettings():
    return LANTERN_GAME.settings

@app.post("/lanterns/settings")
def lanternSettings(settings: LanternSettings):
    LANTERN_GAME.settings = settings
    return LANTERN_GAME.settings

# Laser ------------------------------------------------------------------------

class LaserRegistration(BaseModel):
    enabled: bool

@app.post("/laser/register")
def laserRegister(state: LaserRegistration):
    LASER_GAME.enabled = state.enabled
    LASER_GAME.lastActive = time()
    return {
        "restart": BIG_GAME.restarting
    }

@app.get("/laser/state")
def laserState():
    return {
        "time": time(),
        "lastActive": LASER_GAME.lastActive,
        "enabled": LASER_GAME.enabled
    }

# Chain ------------------------------------------------------------------------

class ChainRegistration(BaseModel):
    enabled: bool
    lastValue: int

@app.post("/chain/register")
def chainRegister(state: ChainRegistration):
    CHAIN_GAME.active = state.enabled
    CHAIN_GAME.lastValue = state.lastValue
    CHAIN_GAME.lastActive = time()
    print(CHAIN_GAME)
    return {}

@app.get("/chain/state")
def chainState():
    CHAIN_GAME.tick()
    return {
        "time": time(),
        "lastActive": CHAIN_GAME.lastActive,
        "active": CHAIN_GAME.active,
        "override": CHAIN_GAME.override,
        "lastValue": CHAIN_GAME.lastValue
    }

@app.post("/chain/overrideOn")
def chainOverrideOn():
    CHAIN_GAME.override = True
    return {}

@app.post("/chain/overrideOff")
def chainOverrideOff():
    CHAIN_GAME.override = False
    return {}


# Cpu --------------------------------------------------------------------------
class CpuRegistration(BaseModel):
    password: str
    fuel: float
    phase: int
    lastCap: int

@app.post("/cpu/register")
def cpuRegister(state: CpuRegistration):
    CPU_GAME.phase = state.phase
    CPU_GAME.lastActive = time()
    CPU_GAME.password = state.password
    CPU_GAME.lastCap = state.lastCap
    CPU_GAME.fuel = state.fuel

    print(state)

    commands = CPU_GAME.commandQueue
    CPU_GAME.commandQueue = []
    print(CPU_GAME.settings)
    return {
        "dischargeRate": CPU_GAME.settings.dischargeRate,
        "overrideFuel": CPU_GAME.settings.overrideFuel,
        "capThreshold": CPU_GAME.settings.capThreshold,
        "commands": commands
    }

@app.post("/cpu/settings")
def cpuSettings(settings: CpuSettings):
    CPU_GAME.settings = settings
    return {}

@app.get("/cpu/settings")
def cpuSettings():
    return CPU_GAME.settings

@app.post("/cpu/command/{command}")
def cpuSettings(command: str):
    CPU_GAME.commandQueue.append(command)
    return {}

@app.get("/cpu/state")
def cpuState():
    val = CPU_GAME.dict()
    val.update({"time": time()})
    return val
