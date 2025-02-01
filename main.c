#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define GAME_TITLE "Asteroids"
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define GAME_WIDTH 640
#define GAME_HEIGHT 360
#define FPS 60
#define PLAYER_SPEED 120
#define PLAYER_ROT_SPEED 250
#define BULLET_SPEED 250
#define CAPACITY 256
#define PADDING 50
#define ASETROIDS_COUNT 10
#define DEBUG false

typedef enum {
	// MENU,
	PLAYING,
	GAME_OVER
} GameState;

typedef struct {
	Vector2 pos;
	float angle;
	float radius;
	int lines;
	int speed;
} Poly;


typedef struct {
	Vector2 pos;
	Vector2 size;
	float angle;
	bool queue_free;
} Bullet;


typedef struct {
	Bullet *items;
	int capacity;
	int count;
} Bullets;


void init_bullets(Bullets *bullets) {
	bullets->items = malloc(CAPACITY * sizeof(Bullet));
	if (bullets->items == NULL) {
		 fprintf(stderr, "Memory allocation failed!\n");
		exit(1);
	}
	bullets->capacity = CAPACITY;
	bullets->count = 0;
}


void append_bullet(Bullets *bullets, Bullet new_bullet) {
    if (bullets->count >= bullets->capacity) {
        int new_capacity = bullets->capacity * 2;
        Bullet *new_items = (Bullet *)realloc(bullets->items, new_capacity * sizeof(Bullet));
        
        if (!new_items) {
            fprintf(stderr, "Memory allocation failed!\n");
            return;
        }
        
        bullets->items = new_items;
        bullets->capacity = new_capacity;
    }
    
    bullets->items[bullets->count] = new_bullet;
    bullets->count++;
}


void remove_bullets(Bullets *bullets) {
	int new_count = 0;
	for(int i = 0; i < bullets->count; i++) {
		Bullet bullet = bullets->items[i];
		if (bullet.queue_free) {
			continue;
		}
		if (new_count != i) {
			bullets->items[new_count] = bullets->items[i];
		}
		new_count++;
	}
	bullets->count = new_count;
}


void player_movement(Poly *player, float dt) {
	if (IsKeyDown(KEY_LEFT)) {
		player->angle -= PLAYER_ROT_SPEED * dt;
	}
	if (IsKeyDown(KEY_RIGHT)) {
		player->angle += PLAYER_ROT_SPEED * dt;
	}

	if (IsKeyDown(KEY_UP)) {
		Vector2 direction = {0};
		float radians = player->angle * DEG2RAD;
		direction.x = cosf(radians);
		direction.y = sinf(radians);

		Vector2 movement = Vector2Scale(direction, PLAYER_SPEED * dt);
		player->pos = Vector2Add(player->pos, movement);
	}

	if (player->pos.x <= 0) {
		player->pos.x = GAME_WIDTH;
	} else if (player->pos.x >= GAME_WIDTH) {
		player->pos.x = 0;
	} else if (player->pos.y <= 0) {
		player->pos.y = GAME_HEIGHT;
	} else if (player->pos.y >= GAME_HEIGHT) {
		player->pos.y = 0;
	}
}

void shoot_bullet(Bullets *bullets, Poly player) {
	Bullet bullet = {0};
	Vector2 direction = {0};
	direction.x = cosf(player.angle * DEG2RAD);
	direction.y = sinf(player.angle * DEG2RAD);
	Vector2 pad = Vector2Scale(direction, 5);
	bullet.pos = Vector2Add(player.pos, pad);
	bullet.size.x = 2;
	bullet.size.y = 6;
	bullet.angle = player.angle;
	bullet.queue_free = false;
	append_bullet(bullets, bullet);
}

void bullet_movement(Bullets bullets, float dt) {
	for(int i = 0; i < bullets.count; i++) {
		Bullet bullet = bullets.items[i];
		float radians = bullet.angle * DEG2RAD;
		Vector2 direction = {0};
		direction.x = cosf(radians);
		direction.y = sinf(radians);

		Vector2 movement = Vector2Scale(direction, BULLET_SPEED * dt);
		bullets.items[i].pos = Vector2Add(bullets.items[i].pos, movement);

		if (bullets.items[i].pos.x <= -PADDING || bullets.items[i].pos.x >= GAME_WIDTH + PADDING || bullets.items[i].pos.y <= -PADDING || bullets.items[i].pos.y >= GAME_HEIGHT + PADDING) {
			bullets.items[i].queue_free = true;
		}
	}
}


void update_asetroids(Poly asteroids[ASETROIDS_COUNT], float dt) {
	for(int i = 0; i < ASETROIDS_COUNT; i++) {
		Vector2 direction = {0};
		direction.x = cosf(asteroids[i].angle * DEG2RAD);
		direction.y = sinf(asteroids[i].angle * DEG2RAD);
		Vector2 movement = Vector2Scale(direction, asteroids[i].speed * dt);
		asteroids[i].pos = Vector2Add(asteroids[i].pos, movement);

		if (asteroids[i].pos.x <= 0 - asteroids[i].radius) {
			asteroids[i].pos.x = GAME_WIDTH + asteroids[i].radius;
		} else if (asteroids[i].pos.x >= GAME_WIDTH + + asteroids[i].radius) {
			asteroids[i].pos.x = 0 - asteroids[i].radius;
		} else if (asteroids[i].pos.y <= 0 - asteroids[i].radius) {
			asteroids[i].pos.y = GAME_HEIGHT + asteroids[i].radius;
		} else if (asteroids[i].pos.y >= GAME_HEIGHT + asteroids[i].radius) {
			asteroids[i].pos.y = 0 - asteroids[i].radius;
		}
	}
}


int main() {

	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
	SetConfigFlags(FLAG_WINDOW_HIGHDPI);
	SetWindowState(FLAG_WINDOW_RESIZABLE);

	SetTargetFPS(FPS);

	RenderTexture2D render_texture = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
	SetTextureFilter(render_texture.texture, TEXTURE_FILTER_POINT);

	// player
	Poly player = {0};
	player.pos.x = GAME_WIDTH/2;
	player.pos.y = GAME_HEIGHT/2;
	player.angle = -90;
	player.radius = 6;
	player.lines = 3;

	Poly asteroids[ASETROIDS_COUNT];
	for(int i = 0; i < ASETROIDS_COUNT; i++) {
		Poly asteroid = {0};
		asteroid.pos.x = GetRandomValue(0, GAME_WIDTH);
		asteroid.pos.y = 0; //GetRandomValue(0, GAME_HEIGHT);
		asteroid.radius = 40.f;
		asteroid.lines = 8;
		asteroid.angle = GetRandomValue(-360, 360);
		asteroid.speed = GetRandomValue(50, 100);
		asteroids[i] = asteroid;
	}

	const float FIRERATE_TIMER = 1/8.f;
	float current_firerate_timer = 0.f;

	Bullets bullets;
	init_bullets(&bullets);

	GameState current_state = PLAYING;


	while(!WindowShouldClose()) {

		float dt = GetFrameTime();

		current_firerate_timer += dt;

		int screen_width = GetScreenWidth();
		int screen_height = GetScreenHeight();

		float scale = fminf(screen_width/(float)GAME_WIDTH, screen_height/(float)GAME_HEIGHT);

		// Update
		switch(current_state) {
			case PLAYING: {
				// player
				player_movement(&player, dt);

				// shoot bullet
				if (IsKeyDown(KEY_SPACE) && current_firerate_timer >= FIRERATE_TIMER)  {
					shoot_bullet(&bullets, player);
					current_firerate_timer = 0.f;
				}

				// update bullets
				bullet_movement(bullets, dt);

				// update asteroids
				update_asetroids(asteroids, dt);

				// bullet asteroid collisions
				for(int i = 0; i < bullets.count; i++) {
					Bullet bullet = bullets.items[i];
					Rectangle bullet_rect = (Rectangle){bullet.pos.x, bullet.pos.y, bullet.size.x, bullet.size.y};
					if (!bullet.queue_free) {
						for(int j = 0; j < ASETROIDS_COUNT; j++) {
							Poly asteroid = asteroids[j];
							float size = asteroid.radius/1.25f;
							Rectangle asteroid_rect = (Rectangle){asteroid.pos.x-size, asteroid.pos.y-size, size*2, size*2};
							if (CheckCollisionRecs(bullet_rect, asteroid_rect)) {
								bullets.items[i].queue_free = true;
								asteroids[j].radius -= 10;
								asteroids[j].lines -= 1;
								asteroids[j].speed += GetRandomValue(10, 20);
								asteroids[j].angle += GetRandomValue(-360, 360);
							} 
						}	
					}
				}

				// check player asetroid collision
				for(int j = 0; j < ASETROIDS_COUNT; j++) {
					Poly asteroid = asteroids[j];
					float a_size = asteroid.radius/1.25f;
					Rectangle asteroid_rect = (Rectangle){asteroid.pos.x-a_size, asteroid.pos.y-a_size, a_size*2, a_size*2};

					float p_size = player.radius/1.5f;
					Rectangle player_rect = (Rectangle){player.pos.x-p_size, player.pos.y-p_size, p_size*2, p_size*2};
					if (CheckCollisionRecs(player_rect, asteroid_rect)) {
						current_state = GAME_OVER;
					} 
				}

				// remove bullets
				remove_bullets(&bullets);
				break;
			}
			case GAME_OVER: {
				if (IsKeyPressed(KEY_ENTER)) {
					current_state = PLAYING;
					for(int i = 0; i < ASETROIDS_COUNT; i++) {
						Poly asteroid = {0};
						asteroid.pos.x = GetRandomValue(0, GAME_WIDTH);
						asteroid.pos.y = 0;
						asteroid.radius = 40.f;
						asteroid.lines = 8;
						asteroid.angle = GetRandomValue(-360, 360);
						asteroid.speed = GetRandomValue(50, 100);
						asteroids[i] = asteroid;
					}
				}
				break;
			}
		}


		BeginTextureMode(render_texture);
		ClearBackground(BLACK);

		switch(current_state) {
			case PLAYING: {
				// draw player
				if (DEBUG) {
					float size = player.radius/1.5f;
					DrawRectangleV((Vector2){player.pos.x-size, player.pos.y-size}, (Vector2){size*2, size*2}, GREEN);	
				}
				DrawPolyLines(player.pos, player.lines, player.radius, player.angle, WHITE);

				// draw bullets
				for(int i = 0; i < bullets.count; i++) {
					Bullet bullet = bullets.items[i];
					Rectangle br = {0};
					br.x = bullet.pos.x;
					br.y = bullet.pos.y;
					br.width = bullet.size.x;
					br.height = bullet.size.y;
					Vector2 origin = {br.width/2, br.height/2};
					DrawRectanglePro(br, origin, bullet.angle-90, WHITE);
				}

				// draw asteroids
				for(int i = 0; i < ASETROIDS_COUNT; i++) {
					Poly asteroid = asteroids[i];
					if (asteroids[i].lines > 4) {
						if (DEBUG) {
							float size = asteroid.radius/1.25f;
							DrawRectangleV((Vector2){asteroid.pos.x-size, asteroid.pos.y-size}, (Vector2){size*2, size*2}, GREEN);	
						}
						DrawPolyLines(asteroid.pos, asteroid.lines, asteroid.radius, asteroid.angle, WHITE);
					}
				}

				if (DEBUG) {
					DrawText(TextFormat("count = %d", bullets.count), 10, 10, 16, RED);	
				}
				break;
			}
			case GAME_OVER: {
				DrawText("GAME OVER: Press [enter] to play again", GAME_WIDTH/2 - 150, GAME_HEIGHT/2.5, 16.f, WHITE);
				break;
			}
		}
		
		EndTextureMode();
		

		{
			// Render target
			Vector2 screen_center = (Vector2){ screen_width / 2, screen_height / 2 };
			Vector2 game_center = (Vector2){ GAME_WIDTH / 2, GAME_HEIGHT / 2 };

			Rectangle src_rect = (Rectangle){ 0, 0, render_texture.texture.width, -render_texture.texture.height };
			Rectangle dest_rect = (Rectangle){ screen_center.x - game_center.x * scale, screen_center.y - game_center.y * scale, GAME_WIDTH * scale, GAME_HEIGHT * scale };

			BeginDrawing();
			ClearBackground(RED);
			DrawTexturePro(render_texture.texture, src_rect, dest_rect, (Vector2){0}, 0.f, WHITE);
			EndDrawing();
		}

	}
	
	free(bullets.items);

	UnloadTexture(render_texture.texture);
	CloseWindow();
	
	return 0;
}