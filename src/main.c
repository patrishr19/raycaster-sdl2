#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>

#define WIDTH 640
#define HEIGHT 640

#define MAP_W 10
#define MAP_H 10

#define CELL_SIZE 64

#define PLAYER_WIDTH 16
#define PLAYER_HEIGHT 16

#define FPS 60

typedef struct {
    float x;
    float y;
    float angle;
} Player;

typedef struct {
    float x, y;
    int side;
} Crossing;

typedef struct {
    Crossing crossings[64];
    int count;
    float hit_x, hit_y;
    float distance;
    int hit_side;
} RayHit;

RayHit cast_ray(Player *p, int map[MAP_H][MAP_W], float angle) {
    RayHit hit = {0};

    float dx = cos(angle);
    float dy = sin(angle);

    float delta_dist_x = fabs(1.0f / dx);
    float delta_dist_y = fabs(1.0f / dy);
    
    int map_x = (int)p->x;
    int map_y = (int)p->y;

    float side_dist_x, side_dist_y;
    int step_x, step_y;
    if (dx > 0) {
	step_x = 1;
	side_dist_x = (map_x + 1.0f - p->x) * delta_dist_x;
    } else {
	step_x = -1;
	side_dist_x = (p->x - map_x) * delta_dist_x;
    } // 0.3105 step -1

    if (dy > 0) {
	step_y = 1;
	side_dist_y = (map_y + 1.0f - p->y) * delta_dist_y;
    } else {
	step_y = -1;
	side_dist_y = (p->y - map_y) * delta_dist_y;
    } // 1.3532 step 1

    while (1) {
	float t;
	int side;

	if (side_dist_x < side_dist_y) {
	    t = side_dist_x; // 0.3105
	    side_dist_x += delta_dist_x; // 0.3105 + 1.155
	    map_x += step_x; // 5 + (-1)
	    side = 0;
	} else {
	    t = side_dist_y;
	    side_dist_y += delta_dist_y;
	    map_y += step_y;
	    side = 1;
	}

	hit.crossings[hit.count++] = (Crossing){
	    .x = p->x + dx * t,
	    .y = p->y + dy * t,
	    .side = side,
	};

	if (map[map_y][map_x] == 1) {
	    hit.hit_x = p->x + dx * t,
	    hit.hit_y = p->y + dy * t,
	    hit.distance = t;
	    hit.hit_side = side;
	    break;
	}
    }
    return hit;
}

void Draw3D(SDL_Renderer *r, Player *p, int map[MAP_H][MAP_W]) {
    int rays_num = 640;
    float fov = M_PI / 3.0f;
    float angle_step = fov / rays_num;
    float angle_start = p->angle - fov / 2.0f;
    int strip_width = WIDTH / rays_num;
    
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_Rect ceiling = {0, 0, WIDTH, HEIGHT / 2};
    SDL_RenderFillRect(r, &ceiling);

    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
    SDL_Rect floor = {0, 0, WIDTH, HEIGHT / 2};
    SDL_RenderFillRect(r, &floor);
    

    for (int i = 0; i < rays_num; i++) {
	float angle = angle_start + i * angle_step;
	RayHit hit = cast_ray(p, map, angle);

	float dist = hit.distance * cos(angle - p->angle);
	printf("dist: %f, hit.distance: %f\n", dist, hit.distance);

	int strip_height = (int)(HEIGHT / dist);

	int top = HEIGHT / 2 - strip_height / 2;
	int bot = HEIGHT / 2 + strip_height / 2;
	int x = i * strip_width;

	int shade = hit.hit_side == 0 ? 255 : 180;

	SDL_SetRenderDrawColor(r, shade, shade, shade, 255);
	SDL_Rect strip = {x, top, strip_width, bot - top};
	SDL_RenderFillRect(r, &strip);
    }
}

void DrawRay(SDL_Renderer *r, Player *p, int map[MAP_H][MAP_W]) {
    int rays_num = 32;
    float fov = M_PI / 3.0f;
    float angle_step = fov / rays_num;
    float angle_start = p->angle - fov / 2.0f;

    for (int i = 0; i < rays_num; i++) {
	float angle = angle_start + i * angle_step;
	RayHit hit = cast_ray(p, map, angle);

	int px = p->x * CELL_SIZE;
	int py = p->y * CELL_SIZE;

	int hx = hit.hit_x * CELL_SIZE;
	int hy = hit.hit_y * CELL_SIZE;
	SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
	SDL_RenderDrawLine(r, px, py, hx, hy);

	for (int i = 0; i < hit.count - 1; i++) {
	    int sx = hit.crossings[i].x * CELL_SIZE;
	    int sy = hit.crossings[i].y * CELL_SIZE;

	    if (hit.crossings[i].side == 0) {
		SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
	    } else {
		SDL_SetRenderDrawColor(r, 0, 0, 255, 255);
	    }

	    SDL_Rect dot = {sx - 3, sy - 3, 6, 6};
	    SDL_RenderFillRect(r, &dot);
	}

	SDL_SetRenderDrawColor(r, 255, 255, 0 , 255);
	SDL_Rect dot = {hx - 4, hy - 4, 8, 8};
	SDL_RenderFillRect(r, &dot);
    }
}

Player PlayerInit() {
    Player p;
    p.x = MAP_W / 2.0f + 0.5f;
    p.y = MAP_H / 2.0f + 0.5f;
    p.angle = 0.0f;
    return p;
}

void DrawPlayer(SDL_Renderer *r, Player *p) {
    int px = p->x * CELL_SIZE;
    int py = p->y * CELL_SIZE;

    SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
    SDL_Rect player = {px - PLAYER_WIDTH/2, py - PLAYER_HEIGHT/2, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderFillRect(r, &player);
}

int map[MAP_H][MAP_W] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 1, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 1, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

void DrawMap(SDL_Renderer *r) {
    for (int y = 0; y < MAP_H; y++) {
	for (int x = 0; x < MAP_W; x++) {
	    if (map[y][x] == 1) {
		SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
	    } else {
		SDL_SetRenderDrawColor(r, 50, 50, 50, 50);
	    }

	    SDL_Rect cell = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
	    SDL_RenderFillRect(r, &cell);
	}
    }
}

void DrawGrid(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    for (int x = 0; x <= MAP_W; x++) {
	SDL_RenderDrawLine(r, x * CELL_SIZE, 0, x * CELL_SIZE, MAP_H * CELL_SIZE);
    }
    for (int y = 0; y <= MAP_H; y++) {
	SDL_RenderDrawLine(r, 0, y * CELL_SIZE, MAP_W * CELL_SIZE, y * CELL_SIZE);
    }
}

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *pwindow = SDL_CreateWindow("raycaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer *prenderer = SDL_CreateRenderer(pwindow, 0, 0);
    SDL_Texture *ptexture = SDL_CreateTexture(prenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    int app_running = true;
    Player player = PlayerInit();

    Uint32 last = SDL_GetTicks();
    int grid_view = 1; 
    while (app_running) {
	Uint32 now = SDL_GetTicks();
	float dt = (now - last) / 1000.0f;
	last = now;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
	    if (event.type == SDL_QUIT) {
		app_running = false;
	    };
	    if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.scancode == SDL_SCANCODE_R) {
		    grid_view = !grid_view;
		}
	    }
	}
	const uint8_t *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT]) player.angle -= 2.0f * dt;
	if (keys[SDL_SCANCODE_RIGHT]) player.angle += 2.0f * dt;
	if (keys[SDL_SCANCODE_W]) {
	    float nx = player.x + cos(player.angle) * 3.0f * dt;
	    float ny = player.y + sin(player.angle) * 3.0f * dt;

	    if (map[(int)ny][(int)nx] == 0) {
		player.x = nx;
		player.y = ny;
	    }
	}
	if (keys[SDL_SCANCODE_S]) {
	    float nx = player.x - cos(player.angle) * 3.0f * dt;
	    float ny = player.y - sin(player.angle) * 3.0f * dt;
	    if (map[(int)ny][(int)nx] == 0) {
		player.x = nx;
		player.y = ny;
	    }
	}
	if (keys[SDL_SCANCODE_A]) {
	    float nx = player.x - cos(player.angle + M_PI / 2.0f) * 3.0f * dt;
	    float ny = player.y - sin(player.angle + M_PI / 2.0f) * 3.0f * dt;
	    if (map[(int)ny][(int)nx] == 0) {
		player.x = nx;
		player.y = ny;
	    }
	}
	if (keys[SDL_SCANCODE_D]) {
	    float nx = player.x - cos(player.angle - M_PI / 2.0f) * 3.0f * dt;
	    float ny = player.y - sin(player.angle - M_PI / 2.0f) * 3.0f * dt;
	    if (map[(int)ny][(int)nx] == 0) {
		player.x = nx;
		player.y = ny;
	    }
	}
	if (keys[SDL_SCANCODE_TAB]) {
	    printf("Player X: %f, Player Y: %f\n", player.x, player.y);
	    printf("dx: %f, dy: %f\n", cos(player.angle), sin(player.angle));
	    printf("fabx: %f, faby: %f\n", fabs( 1.0f / cos(player.angle)), fabs( 1.0f / sin(player.angle)));
	}

	SDL_SetRenderDrawColor(prenderer, 0, 0, 0, 255);
	SDL_RenderClear(prenderer);

	if (grid_view) {
	    DrawMap(prenderer);
	    DrawGrid(prenderer);
	    DrawRay(prenderer, &player, map);
	    DrawPlayer(prenderer, &player);
	} else {
	    Draw3D(prenderer, &player, map);
	}

	SDL_RenderPresent(prenderer);

	Uint32 frame_time = SDL_GetTicks() - now;
	Uint32 target_ms = 1000 / FPS;
	if (frame_time < target_ms) {
	    SDL_Delay(target_ms - frame_time);
	}
	SDL_Delay(1);
    }

    SDL_DestroyTexture(ptexture);
    SDL_DestroyRenderer(prenderer);
    SDL_DestroyWindow(pwindow);
    SDL_Quit();

    return 0;
}
