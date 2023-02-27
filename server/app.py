from fastapi import FastAPI
from pydantic import BaseModel, PrivateAttr
from typing import Dict
from time import time

app = FastAPI()

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

BUTTONS_GAME = ButtonsGame()


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
        "buttons": BUTTONS_GAME.buttons
    }
