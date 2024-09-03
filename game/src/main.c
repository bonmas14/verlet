/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

-- Copyright (c) 2020-2024 Jeffery Myers
--
--This software is provided "as-is", without any express or implied warranty. In no event 
--will the authors be held liable for any damages arising from the use of this software.

--Permission is granted to anyone to use this software for any purpose, including commercial 
--applications, and to alter it and redistribute it freely, subject to the following restrictions:

--  1. The origin of this software must not be misrepresented; you must not claim that you 
--  wrote the original software. If you use this software in a product, an acknowledgment 
--  in the product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented
--  as being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.

*/

#include "raylib.h"
#include "raymath.h"

#include "stddef.h"
#include "stdbool.h"
#include "stdint.h"

#include "game.h"   // an external header in this project
#include "lib.h"	// an external header in the static lib project

typedef struct verlet_t {
    Vector2 accel, prev, curr;
    float timer;
} verlet_t;

typedef struct properties_t {
    bool constant;
} properties_t;

#define ENT_COUNT 1024

#define GRAVITY (Vector2) { .y = 9.81 } 
#define TIMESTAMP (1.0 / 144.0)
#define STEPS_PER_FRAME 1

#define SQR_TIMESTAMP (TIMESTAMP * TIMESTAMP)

typedef struct entities_t {
    bool         occupied[ENT_COUNT];
    verlet_t     entities[ENT_COUNT];
    properties_t prop[ENT_COUNT];
} entities_t;

entities_t *state;

Camera2D cam = { 0 };


bool AddPoint(verlet_t *point, bool is_const) {
    for (size_t i = 0; i < ENT_COUNT; i++) {
        if (!state->occupied[i]) {
            state->occupied[i] = true;
            state->entities[i] = *point;
            state->prop[i].constant = is_const;

            return true;
        }
    }

    return false;
}

void GameInit() {
    cam.target = (Vector2){ 0 };
    cam.offset = (Vector2){ InitalWidth / 2.0f, InitalHeight / 2.0f };
    cam.rotation = 0.0f;
    cam.zoom = 4.0f;

    // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(InitalWidth, InitalHeight, "Example");
    SetTargetFPS(144);

    state = (entities_t*)MemAlloc(sizeof(entities_t));

    if (state == NULL) {
        TraceLog(LOG_ERROR, "MemAlloc error. No memory.");
        CloseWindow();
        return;
    }

    for (size_t i = 0; i < ENT_COUNT; i++) {
        state->occupied[i] = false;
        state->prop[i]     = (properties_t) { 0 };
    }

    size_t width  = 10;
    size_t height = 10; // 100

    for (size_t i = 0; i < (width * height); i++) {
        size_t x = i % width;
        size_t y = i / width;

        float xPos = (x - width  / 2.0) * 10.0;
        float yPos = (y - height / 2.0) * 10.0;

        verlet_t point = (verlet_t) {
            .curr = (Vector2) {xPos, yPos},
            .prev = (Vector2) {xPos, yPos - 0.1} 
        };
        bool is_const = y == 0;

        AddPoint(&point, is_const);
    }
}

void GameCleanup() {
    MemFree(state);

    CloseWindow();
}

bool GameUpdate() {
    for (size_t j = 0; j < STEPS_PER_FRAME; j++) {
        for (size_t i = 0; i < ENT_COUNT; i++) {
            if (!state->occupied[i]) continue;
            if (state->prop[i].constant) continue;

            verlet_t *point = &state->entities[i];

            Vector2 disp = Vector2Subtract(point->curr, point->prev);
            point->prev  = point->curr;
            point->curr  = Vector2Add(point->curr, disp);

            // point->accel = Vector2Add(point->accel,   GRAVITY);
            // point->accel = Vector2Scale(point->accel, SQR_TIMESTAMP);
            // point->curr  = Vector2Add(point->curr,    point->accel);
            // point->accel = (Vector2) { 0 };
        }
    }


    return true;
}

void GameDraw() {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode2D(cam);

    for (size_t i = 0; i < ENT_COUNT; i++) {
        if (!state->occupied[i]) continue;

        verlet_t point = state->entities[i];
        DrawCircleV(point.curr, 1, WHITE);
    }

    DrawRectangle(0, 0, 1, 16, WHITE);

    EndMode2D();

    DrawFPS(0, 0);
    EndDrawing();
}

int main() {
    GameInit();

    while (!WindowShouldClose()) {
        if (!GameUpdate())
            break;

        GameDraw();
    }

    GameCleanup();

    return 0;
}
