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

#define ENT_COUNT 1024
#define CONSTR_COUNT 4096

#define GRAVITY (Vector2) { .x = 0, .y = 9.81 } 

#define FPS_LIM 60.0
#define TIMESTAMP (1.0 / FPS_LIM)
#define STEPS_PER_FRAME 2
#define STEPS_FOR_CONSTR_SATIS 4

#define SQR_TIMESTAMP (TIMESTAMP * TIMESTAMP)

typedef struct verlet_t {
    Vector2 prev, curr;
    float timer;
} verlet_t;

typedef enum constrain_types_t {
    CONSTR_NONE = 0,
    CONSTR_LINE,
    CONSTR_SPRING,
    CONSTR_ROPE,
} constrain_types_t;

typedef struct constrain_t {
    constrain_types_t type;
    float             distance;

    int32_t pointA;
    int32_t pointB;
} constrain_t;

typedef struct properties_t {
    bool constant;
} properties_t;


typedef struct points_t {
    bool         occupied[ENT_COUNT];
    verlet_t     entities[ENT_COUNT];
    properties_t prop[ENT_COUNT];
} points_t;

typedef struct constrains_t {
    bool        occupied[CONSTR_COUNT];
    constrain_t constrains[CONSTR_COUNT];
} constrains_t;

points_t     *points;
constrains_t *constrains;
Camera2D      cam = { 0 };

int32_t AddPoint(verlet_t *point, bool is_const) {
    for (int32_t i = 0; i < ENT_COUNT; i++) {
        if (!points->occupied[i]) {
            points->occupied[i] = true;
            points->entities[i] = *point;
            points->prop[i].constant = is_const;

            return i;
        }
    }

    return -1;
}

int32_t AddConstrain(constrain_t *constr) {
    for (int32_t i = 0; i < CONSTR_COUNT; i++) {
        if (!constrains->occupied[i]) {
            constrains->occupied[i] = true;
            constrains->constrains[i] = *constr;
            return i;
        }
    }

    return -1;
}

void GameInit() {
    cam.target = (Vector2){ 0 };
    cam.offset = (Vector2){ InitalWidth / 2.0f, InitalHeight / 2.0f };
    cam.rotation = 0.0f;
    cam.zoom = 4.0f;

    // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(InitalWidth, InitalHeight, "Example");
    SetTargetFPS(FPS_LIM * STEPS_PER_FRAME);

    points     = (points_t*)    MemAlloc(sizeof(points_t));
    constrains = (constrains_t*)MemAlloc(sizeof(constrains_t));

    if (points == NULL || constrains == NULL) {
        TraceLog(LOG_ERROR, "MemAlloc error. No memory.");
        CloseWindow();
        return;
    }

    for (size_t i = 0; i < ENT_COUNT; i++) {
        points->occupied[i] = false;
        points->prop[i]     = (properties_t) { 0 };
    }

    for (size_t i = 0; i < CONSTR_COUNT; i++) {
        constrains->occupied[i] = false;
    }

    size_t width  = 4 * 4;
    size_t height = 5; // 100

    float distance = 10;
    size_t start = 0;

    for (size_t i = 0; i < (width * height); i++) {
        size_t x = i % width;
        size_t y = i / width;

        float xPos = (x - (width  - 1) / 2.0) * distance;
        float yPos = (y - (height - 1) / 2.0) * distance;

        verlet_t pointA = (verlet_t) {
            .curr = (Vector2) { xPos, yPos },
            .prev = (Vector2) { xPos, yPos } 
        };

        bool is_const = y == 0 && (x == 0 || x == (width - 1) || (x % 4) == 0);

        size_t index = AddPoint(&pointA, is_const);

        if (i == 0) start = index;
    }

    for (size_t i = 0; i < (width * height); i++) {
        size_t x = i % width;
        size_t y = i / width;

        constrain_t constrV = {
            .type = CONSTR_ROPE,
            .distance = distance,

            .pointA = start + i,
            .pointB = start + width + i,
        };

        constrain_t constrH = {
            .type = CONSTR_ROPE,
            .distance = distance,

            .pointA = start + i,
            .pointB = start + i + 1,
        };

        if (x != (width - 1)) {
            AddConstrain(&constrH);
        }

        if (y != (height - 1)) {
            AddConstrain(&constrV);
        }
    }


}

void GameCleanup() {
    MemFree(points);

    CloseWindow();
}

void satisfy_constrain(constrain_t *cond) {
    verlet_t *pointA = &points->entities[cond->pointA];
    verlet_t *pointB = &points->entities[cond->pointB];

    if (points->prop[cond->pointA].constant && points->prop[cond->pointB].constant) {
        return;
    }

    float distance = Vector2Distance(pointA->curr, pointB->curr);
    Vector2 dir = Vector2Normalize(Vector2Subtract(pointA->curr, pointB->curr));

    float diff  = distance - cond->distance;

    if (cond->type == CONSTR_ROPE && diff < 0) {
        return;
    }


    if (points->prop[cond->pointA].constant) {
        pointB->curr = Vector2Add(pointB->curr, Vector2Scale(dir, diff));
        return;
    }

    if (points->prop[cond->pointB].constant) {
        pointA->curr = Vector2Add(pointA->curr, Vector2Scale(dir, -diff));
        return;
    }

    // THIS IS ONLY / 2 if mass is same

    pointA->curr = Vector2Add(pointA->curr, Vector2Scale(dir, -diff / 2.0f));
    pointB->curr = Vector2Add(pointB->curr, Vector2Scale(dir,  diff / 2.0f));
}

bool GameUpdate() {
    for (size_t j = 0; j < STEPS_PER_FRAME; j++) {
        for (size_t i = 0; i < ENT_COUNT; i++) {
            if (!points->occupied[i]) continue;
            if (points->prop[i].constant) continue;

            verlet_t *point = &points->entities[i];

            Vector2 disp = Vector2Subtract(point->curr, point->prev);
            point->prev  = point->curr;
            point->curr  = Vector2Add(point->curr, disp);

            Vector2 accel = GRAVITY;

            if (IsKeyDown(KEY_A))
                accel = Vector2Add(accel, (Vector2) { .x = -10 });

            if (IsKeyDown(KEY_D))
                accel = Vector2Add(accel, (Vector2) { .x =  10 });
            
            if (IsKeyDown(KEY_W))
                accel = Vector2Add(accel, (Vector2) { .y = -10 });

            if (IsKeyDown(KEY_S))
                accel = Vector2Add(accel, (Vector2) { .y =  10 });

            accel         = Vector2Scale(accel, SQR_TIMESTAMP);
            point->curr   = Vector2Add(point->curr, accel);
        }

        for (size_t k = 0; k < STEPS_FOR_CONSTR_SATIS; k++) {
            for (size_t i = 0; i < CONSTR_COUNT; i++) {
                if (!constrains->occupied[i]) continue;
                constrain_t *cond = &constrains->constrains[i];
                satisfy_constrain(cond);
            }
        }
    }


    return true;
}

void GameDraw() {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode2D(cam);

    for (size_t i = 0; i < ENT_COUNT; i++) {
        if (!points->occupied[i]) continue;

        verlet_t point = points->entities[i];
        DrawCircleV(point.curr, 1, WHITE);
    }

    for (size_t i = 0; i < CONSTR_COUNT; i++) {
        if (!constrains->occupied[i]) continue;
        constrain_t *cond = &constrains->constrains[i];

        verlet_t *pointA = &points->entities[cond->pointA];
        verlet_t *pointB = &points->entities[cond->pointB];

        DrawLineV(pointA->curr, pointB->curr, WHITE);
    }

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
