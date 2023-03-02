#!/usr/bin/env sh

(cd server && uvicorn app:app --reload --host 0.0.0.0)
