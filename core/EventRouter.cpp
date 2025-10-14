#include "EventRouter.h"
#include <iostream>
#include <chrono>

// Same rectangle as main.cpp for now
static const float exitX = 10, exitY = 10, exitW = 10, exitH = 10;

EventRouter::EventRouter(bool* runningFlag)
    : running(runningFlag) {
    std::cout << "[EventRouter] Initialized.\n";
}

EventRouter::~EventRouter() {
    std::cout << "[EventRouter] Destroyed.\n";
}

void EventRouter::processEvent(const SDL_Event& e) {
    switch (e.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            logKeyEvent(e.key);
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            logMouseEvent(e.button);
            break;

        case SDL_MOUSEMOTION:
            // no logging, ignore mouse motion
            break;

        case SDL_FINGERDOWN:
        case SDL_FINGERUP:
        case SDL_FINGERMOTION:
            logTouchEvent(e.tfinger);
            break;

        default:
            break;
    }
}

void EventRouter::logKeyEvent(const SDL_KeyboardEvent& key) {
    // std::string state = (key.type == SDL_KEYDOWN) ? "DOWN" : "UP";
    // std::cout << "[Key] scancode=" << key.keysym.scancode
    //           << " keycode=" << key.keysym.sym
    //           << " state=" << state << "\n";
}

void EventRouter::logMouseEvent(const SDL_MouseButtonEvent& btn) {
    // Exit button hit-test only
    if (btn.type == SDL_MOUSEBUTTONDOWN) {
        if (btn.x >= exitX && btn.x <= exitX + exitW &&
            btn.y >= exitY && btn.y <= exitY + exitH) {
            // std::cout << "[Exit] Button clicked — quitting.\n";
            *running = false;
        }
    }
}


void EventRouter::logTouchEvent(const SDL_TouchFingerEvent& touch) {
    using clock = std::chrono::high_resolution_clock;
    auto now = clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::microseconds>(
                   now.time_since_epoch()).count();

    std::string phase;
    if (touch.type == SDL_FINGERDOWN) {
        phase = "start";
        touches[touch.fingerId] = {touch.x, touch.y};
    } else if (touch.type == SDL_FINGERMOTION) {
        phase = "move";
        touches[touch.fingerId] = {touch.x, touch.y};
    } else if (touch.type == SDL_FINGERUP) {
        phase = "end";
        touches.erase(touch.fingerId);
    }

    updateTouchLabel();

    // std::cout << "[Touch] id=" << touch.fingerId
    //         << " phase=" << (touch.type == SDL_FINGERDOWN ? "start" :
    //                         touch.type == SDL_FINGERMOTION ? "move" : "end")
    //         << " x=" << touch.x
    //         << " y=" << touch.y
    //         << " active=" << touches.size()
    //         << " t=" << ms << " µs\n";

}




void EventRouter::updateTouchLabel() {
    std::string txt;
    for (auto& [id, pos] : touches) {
        if (!txt.empty()) txt += " | ";
        txt += "id" + std::to_string(id) + ":" +
               std::to_string((int)(pos.first * 1000)) + "," +
               std::to_string((int)(pos.second * 1000));
    }

    if (touchUpdateCallback) {
        touchUpdateCallback(txt.empty() ? "No touches" : txt);
    }
}
