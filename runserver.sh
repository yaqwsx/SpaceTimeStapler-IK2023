#!/usr/bin/env sh

(cd server && uvicorn app:app --reload)
