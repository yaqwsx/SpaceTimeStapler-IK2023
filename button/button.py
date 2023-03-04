#!/usr/bin/env python3

import requests
import argparse
from timeit import default_timer as timer
import time
import os
import _thread

class Every:
    def __init__(self, period):
        self.period = period
        self.last = 0

    def trigger(self):
        t = timer()
        ret = t - self.last > self.period
        if ret:
            self.last = t
        return ret

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--soft", help="Do not use HW button", action="store_true")
    parser.add_argument("-u", "--url", help="url for the button server")
    parser.add_argument("-d", "--direct", help="direct url for the server")
    args = parser.parse_args()

    if args.soft:
        import keyboard
    else:
        import gpiozero

    if args.url is not None:
        x = requests.get(args.url).text
    elif args.direct is not None:
        x = args.direct

    server = x.strip()
    if not server.startswith("http://"):
        server = "http://" + server

    print(server)
    print("Registering... ", end="")
    x = requests.get(server + "/register")
    id = x.json()["id"]
    print(f"got id: {id}")

    register_timer = Every(2)
    button_deadtime = Every(10)
    status_time = Every(0.5)
    decision_deadline = 0
    decision_time = 5

    if args.soft:
        button = lambda: not keyboard.is_pressed("space")
        reveal = lambda x, _: print("Revealed") if x["revealed"] else None
    else:
        btn = gpiozero.Button(26)
        led_g = gpiozero.LED(16)
        led_r = gpiozero.LED(20)
        def r(state, limit):
            if state["revealed"]:
                led_g.on()
                led_r.off()
            else:
                if timer() < limit:
                    led_g.on()
                    led_r.on()
                else:
                    led_g.off()
                    led_r.on()

        button = lambda: btn.is_pressed
        reveal = r

        for i in range(10):
            l = led_r if i % 2 == 0 else led_g
            l.on()
            time.sleep(0.3)
            l.off()

    status = requests.get(server + "/decision").json()
    def update_status():
        try:
            while True:
                if status_time.trigger():
                    x = requests.get(server + "/decision")
                    status = x.json()
                    decision_deadline = x.json()["pressTolerance"]
                    reveal(status, decision_deadline)
                if register_timer.trigger():
                    requests.post(server + f"/{id}/register")
                time.sleep(0.1)
        except Exception as e:
            print(e)
            led_g.off()
            led_r.off()
            os.kill(os.getpid(), signal.SIGINT)
    _thread.start_new_thread(update_status, ())
    while True:
        if not button() and button_deadtime.trigger():
            time.sleep(0.1)
            print("Button was pressed!")
            decision_deadline = timer() + decision_time + 1
            reveal(status, decision_deadline)
            requests.post(server + f"/{id}/press")

