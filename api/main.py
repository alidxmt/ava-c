import os
from fastapi import FastAPI, HTTPException, Body
from fastapi.responses import JSONResponse
import json

app = FastAPI(title="AVA JSON Server")

USERS_FILE = "users.json"
JSON_FOLDER = os.path.dirname(os.path.abspath(__file__))

# --- Allowed JSON files ---
ALLOWED_FILES = ["dastan", "modes"]  # add all permitted files here

# --- Utility ---
def load_json(file_path: str):
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"{file_path} not found")
    with open(file_path, "r", encoding="utf-8") as f:
        return json.load(f)

# --- Endpoint ---
@app.post("/api/get_json")
def get_json(data: dict = Body(...)):
    name = data.get("name")
    pin = data.get("pin")
    file = data.get("file", "dastan")

    # --- Validate inputs ---
    if not name or not pin:
        raise HTTPException(status_code=400, detail="Missing name or PIN")
    if file not in ALLOWED_FILES:
        raise HTTPException(status_code=400, detail="Invalid file requested")

    # --- Load users and check credentials ---
    users = load_json(USERS_FILE)
    user = users.get(name)
    if not user or user["pin"] != pin:
        raise HTTPException(status_code=401, detail="Invalid name or PIN")

    # --- Load requested JSON file ---
    file_path = os.path.join(JSON_FOLDER, f"{file}.json")
    try:
        data = load_json(file_path)
    except FileNotFoundError:
        raise HTTPException(status_code=404, detail=f"{file}.json not found")

    return JSONResponse(content=data)

# --- Root ---
@app.get("/")
def root():
    available_files = [f.replace(".json","") for f in os.listdir(JSON_FOLDER) if f.endswith(".json")]
    users = load_json(USERS_FILE)
    return {
        "message": "AVA JSON API running",
        "available_files": available_files,
        "registered_users": list(users.keys())
    }
