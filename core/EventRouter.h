#pragma once
#include <SDL.h>
#include <iostream>
#include <string>
#include <map>
#include <functional>   



class EventRouter {
public:
    explicit EventRouter(bool* runningFlag);
    ~EventRouter();

    void processEvent(const SDL_Event& e);

    // Callback setter
    void setTouchUpdateCallback(std::function<void(const std::string&)> cb) {
        touchUpdateCallback = std::move(cb);
    }

private:
    bool* running;
    std::map<SDL_FingerID, std::pair<float,float>> touches;

    std::function<void(const std::string&)> touchUpdateCallback;  // 👈 here

    void logKeyEvent(const SDL_KeyboardEvent& key);
    void logMouseEvent(const SDL_MouseButtonEvent& btn);
    void logMouseMotion(const SDL_MouseMotionEvent& motion);
    void logTouchEvent(const SDL_TouchFingerEvent& touch);
    void updateTouchLabel();
};
