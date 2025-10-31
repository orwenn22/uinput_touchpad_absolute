#include "VisualizerThread.h"

#include <cstdio>
#include <deque>
#include <raylib.h>

#include "../MainThread.h"

std::atomic<bool> VisualizerThread::s_already_running = false;


struct VisualizerParticle {
    Vector2 position;
    Color color;
    float time;

    VisualizerParticle(const Vector2 &position, Color color) : position(position), color(color), time(.25f) {}
    ~VisualizerParticle() = default;

    void Update(float dt) {
        time -= dt;
    }

    void Render() const {
        //DrawRectangle(position.x, position.y, 5, 5, color);
        DrawCircleV(position, time/.25f * 3.f, {color.r, color.g, color.b, (unsigned char)((time/.25f) * 255.f)});
    }
};


VisualizerThread::VisualizerThread() : SecondaryThread(2) {
}

VisualizerThread::~VisualizerThread() = default;

void VisualizerThread::Run() {
    if (s_already_running) return;

    int screen_width = 300;
    int screen_height = 175;

    s_already_running = true;
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
    SetTraceLogLevel(LOG_NONE);
    InitWindow(screen_width, screen_height, "Visualizer");
    SetTargetFPS(144);

    AbsoluteTouchMouse *device = GetMainThread()->GetAbsoluteTouchMouse();

    std::deque<VisualizerParticle> particles;

    float particle_time = 0;

    while (!WindowShouldClose() && Running() && GetMainThread()->m_running) {
        //update
        device->positions_lock.lock();
        int absolute_max_x = device->absolute_max_x; //todo: these two are technically not associated with the positions_lock mutex
        int absolute_max_y = device->absolute_max_y;
        int absolute_x = device->absolute_x;
        int absolute_y = device->absolute_y;
        int clamped_absolute_x = device->clamped_absolute_x;
        int clamped_absolute_y = device->clamped_absolute_y;
        int area_min_x = device->area_min_x;
        int area_min_y = device->area_min_y;
        int area_max_x = device->area_max_x;
        int area_max_y = device->area_max_y;
        bool area_enabled = device->enable_area;
        device->positions_lock.unlock();

        Rectangle trackpad = {
            15.f, 15.f,
            (float)screen_width - 15.f*2.f, (float)screen_height - 15.f*2.f
        };
        Rectangle area_rec = {
            trackpad.x + (float)area_min_x/(float)absolute_max_x * trackpad.width,
            trackpad.y + (float)area_min_y/(float)absolute_max_y * trackpad.height,
            (float)(area_max_x-area_min_x)/(float)absolute_max_x * trackpad.width,
            (float)(area_max_y-area_min_y)/(float)absolute_max_y * trackpad.height,
        };

        Vector2 absolute_pos = {
            trackpad.width * (float)absolute_x / (float)absolute_max_x + trackpad.x,
            trackpad.height * (float)absolute_y / (float)absolute_max_y + trackpad.y
        };
        Vector2 clamped_pos = {
            trackpad.width * (float)clamped_absolute_x / (float)absolute_max_x + trackpad.x,
            trackpad.height * (float)clamped_absolute_y / (float)absolute_max_y + trackpad.y
        };

        float dt = GetFrameTime();
        //particle_time += dt;
        //if (particle_time >= 0.025f) {
            particles.emplace_back(clamped_pos, GREEN);
        //    particle_time -= 0.025f;
        //}
        for (auto it = particles.begin(); it != particles.end(); /*nothing*/) {
            it->Update(dt);
            if (it->time <= 0) it = particles.erase(it);
            else ++it;
        }


        //draw
        BeginDrawing();
        ClearBackground(BLANK);
        if (area_enabled) DrawRectangleRoundedLinesEx(area_rec, 0.01f, 2, 2.f, GRAY);
        DrawRectangleRoundedLinesEx(trackpad, 0.01f, 2, 2.f, WHITE);

        DrawText(TextFormat("%i %i", clamped_absolute_x, clamped_absolute_y), 0, 0, 10, GREEN);
        DrawText(TextFormat("%i %i", absolute_max_x, absolute_max_y), screen_width-60, screen_height-10, 10, WHITE);
        if (area_enabled) {
            DrawText(TextFormat("%i %i", absolute_x, absolute_y), 75, 0, 10, YELLOW);
            DrawText(TextFormat("area : %i %i to %i %i", area_min_x, area_min_y, area_max_x, area_max_y), 0, screen_height-10, 10, GRAY);
        }

        //FIXME: THIS CAUSES THE PROGRAM TO CRASH AT HIGH FRAMERATE AFTER A WHILE
        //this is most likely because raylib (or mesa/libgallium?) isn't meant to be funy used on another thread
        for (auto &particle : particles) particle.Render();

        //yellow dot (absolute pos)
        DrawCircleV(absolute_pos, 5.f, YELLOW);

        //green dot (clamped absolute pos)
        DrawCircleV(clamped_pos, 5.f, GREEN);

        EndDrawing();
    }

    CloseWindow();
    s_already_running = false;
    printf("VisualizerThread end\n");
}
