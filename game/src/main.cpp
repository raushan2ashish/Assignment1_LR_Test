#include <raylib.h>
#include <raymath.h>

#include <cassert>
#include <array>
#include <vector>
#include <algorithm>

constexpr float SCREEN_SIZE = 800;
constexpr int TILE_COUNT = 20;
constexpr float TILE_SIZE = SCREEN_SIZE / TILE_COUNT;

constexpr float BULLET_RADIUS = 10.0f;
constexpr float BULLET_SPEED = 400.0f;
constexpr float BULLET_LIFE_TIME = 1.0f;
constexpr int BULLET_DAMAGE = 25;// ---> damage for each bullet <---

constexpr float TURRET_RADIUS = TILE_SIZE * 0.4f;// ---> added radius for turrets <---
constexpr float TURRET_RANGE = 200.0f; //---> this defines the ranfe of the turrets <---
constexpr float TURRET_SHOOT_COOLDOWN = 0.5f; //---> cooldown time for the turrets <---
constexpr int MAX_TURRETS = 5;                   // ---> added number of maximum turrets <---
constexpr float ENEMY_RADIUS = TILE_SIZE * 0.5f;// ---> radius for enemy <---

enum TileType : int
{
    GRASS,      // Marks unoccupied space, can be overwritten 
    DIRT,       // Marks the path, cannot be overwritten
    WAYPOINT,   // Marks where the path turns, cannot be overwritten
    COUNT
};

struct Cell
{
    int row;
    int col;
};

constexpr std::array<Cell, 4> DIRECTIONS{ Cell{ -1, 0 }, Cell{ 1, 0 }, Cell{ 0, -1 }, Cell{ 0, 1 } };
// --->added enums for different stages for game,for now adding just two phases <--- 
enum GameState
{
	STRATEGY_PHASE,   //--->player will place turrets in this phase<---
	COMBAT_PHASE,   //--->enemies will move and turrets will shoot in this phase<---
    LEVEL_WON,
    LEVEL_LOST
};

// --->we will define different types of enemy here<---
enum EnemyType
{
    NORMAL, 
	FAST,       // ---> will move faster than the normal one <---
	HEAVY,      // ---> will move slowely but have more health <---
};

inline bool InBounds(Cell cell, int rows = TILE_COUNT, int cols = TILE_COUNT)
{
    return cell.col >= 0 && cell.col < cols && cell.row >= 0 && cell.row < rows;
}

void DrawTile(int row, int col, Color color)
{
    DrawRectangle(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
}

void DrawTile(int row, int col, int type)
{
    Color colors[COUNT];
    colors[GRASS] = LIME;
    colors[DIRT] = BEIGE;
    colors[WAYPOINT] = SKYBLUE;
    //assert(type >= 0 && type < COUNT);
    DrawTile(row, col, colors[type]);
}

Vector2 TileCenter(int row, int col)
{
    float x = col * TILE_SIZE + TILE_SIZE * 0.5f;
    float y = row * TILE_SIZE + TILE_SIZE * 0.5f;
    return { x, y };
}

// Returns a collection of adjacent cells that match the search value.
std::vector<Cell> FloodFill(Cell start, int tiles[TILE_COUNT][TILE_COUNT], TileType searchValue)
{
    // "open" = "places we want to search", "closed" = "places we've already searched".
    std::vector<Cell> result;
    std::vector<Cell> open;
    bool closed[TILE_COUNT][TILE_COUNT];
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            // We don't want to search zero-tiles, so add them to closed!
            closed[row][col] = tiles[row][col] == 0;
        }
    }

    // Add the starting cell to the exploration queue & search till there's nothing left!
    open.push_back(start);
    while (!open.empty())
    {
        // Remove from queue and prevent revisiting
        Cell cell = open.back();
        open.pop_back();
        closed[cell.row][cell.col] = true;

        // Add to result if explored cell has the desired value
        if (tiles[cell.row][cell.col] == searchValue)
            result.push_back(cell);

        // Search neighbours
        for (Cell dir : DIRECTIONS)
        {
            Cell adj = { cell.row + dir.row, cell.col + dir.col };
            if (InBounds(adj) && !closed[adj.row][adj.col] && tiles[adj.row][adj.col] != 0)
                open.push_back(adj);
        }
    }

    return result;
}


struct Bullet
{
    Vector2 position = Vector2Zeros;
    Vector2 direction = Vector2Zeros;
    float time = 0.0f;
    bool destroy = false;
};

// ---> added Turret struct <---
struct Turret
{
    Vector2 position = { 0, 0 };
    float shootTimer = 0.0f;// ---> added shooting countdown for each turret <---
};
// ---> added Enemy struct<---
struct Enemy
{
    EnemyType type;
    Vector2 position = { 0, 0 };
    int health = 100;
    float speed = 100.0f;
    int waypointIndex = 0;
    bool shouldBeDestroyed = false;
};
Enemy CreateEnemy(EnemyType type, Vector2 startPos, int level)
{
    Enemy enemy;
    enemy.type = type;
    enemy.position = startPos;
    enemy.waypointIndex = 0; //--->to make sure , enemy start from 1st waypoint <---
	//---> multiplying health and speed to increase the level of difficuilty <---
    float healthMultiplier = 1.0f + (level - 1) * 0.5f; 
    float speedMultiplier = 1.0f + (level - 1) * 0.25f; 
    switch (type)
    {
    case NORMAL:
        enemy.health = 100 * healthMultiplier;
        enemy.speed = 80.0f  * speedMultiplier;
        break;

    case FAST:
        enemy.health = 75 * healthMultiplier;
        enemy.speed = 150.0f * speedMultiplier;
        break;
    case HEAVY:
        enemy.health = 300 * healthMultiplier;
        enemy.speed = 50.0f * speedMultiplier;
        break; 
    }
    return enemy;
}
int main()
{
    int tiles[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 }, // 0
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 1
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 2
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 4
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 5
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 6
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0 }, // 7
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 10
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 11
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 12
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 13
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 14
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 15
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 16
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 18
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // 19
    };
    std::vector<Cell> waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT);
	//---> enemy wave coming in first level, all will be normal enemies <---
    std::vector<EnemyType> level1_wave = { NORMAL, NORMAL, FAST, NORMAL, HEAVY, FAST, NORMAL, NORMAL, FAST, HEAVY };
    // ---> adding new variable for new levels <---
    std::vector<EnemyType> level2_wave = { FAST, FAST, FAST, HEAVY, HEAVY, NORMAL, NORMAL, FAST, HEAVY, FAST, FAST, HEAVY };
    std::vector<EnemyType> level3_wave = { HEAVY, HEAVY, HEAVY, FAST, FAST, NORMAL, NORMAL, FAST, HEAVY, HEAVY, FAST, NORMAL, HEAVY, FAST, HEAVY };
    std::vector<EnemyType> current_wave;

    int currentLevel = 1;

    // ---> for first enemy <---
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Turret> turrets;// ---> added for turrets <---

    // ---> enemy span <---
    GameState currentState = STRATEGY_PHASE;
    int enemiesToSpawn = 0;
    int enemiesSpawned = 0;
    float spawnTimer = 0.0f;
	float spawnInterval = 1.0f; // ---> enemy spawn after second <---

    InitWindow(SCREEN_SIZE, SCREEN_SIZE, "Tower Defense");
    InitAudioDevice();
	//---> load sounds <---
    Sound turretCreateSound = LoadSound("create.wav");
    Sound turretDestroySound = LoadSound("destroy.wav");
    Sound turretShootSound = LoadSound("shoot.wav");
    Sound enemyHitSound = LoadSound("hit.wav");
    Sound enemyDeathSound = LoadSound("death.wav");
	//---> load textures/Sprites <---
    Texture2D turretTexture = LoadTexture("turret.png");
    Texture2D bulletTexture = LoadTexture("bullet.png");
    Texture2D enemyNormalTexture = LoadTexture("enemy_normal.png");
    Texture2D enemyFastTexture = LoadTexture("enemy_fast.png");
    Texture2D enemyHeavyTexture = LoadTexture("enemy_heavy.png");

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        
        switch (currentState)
        {
            case STRATEGY_PHASE:
            { 
                if (turrets.size() < MAX_TURRETS)         //--->added click left mouse to create turret<---start
                {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    {
                        Vector2 mousePos = GetMousePosition();
                        int col = mousePos.x / TILE_SIZE;
                        int row = mousePos.y / TILE_SIZE;

                        // only be placed on grass, will not be placed on dirt or waypoint
                        if (InBounds({ row, col }) && tiles[row][col] == GRASS)
                        {
                            Turret newTurret;
                            newTurret.position = TileCenter(row, col);
                            turrets.push_back(newTurret);
							PlaySound(turretCreateSound);//--->play sound when turret is created<---
                        }
                    }
                }
				if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))//---> added right click to destroy turret <---
                {
                    Vector2 mousePos = GetMousePosition();
                    for (int i = 0; i < turrets.size(); ++i)
                    {
                        if (CheckCollisionPointCircle(mousePos, turrets[i].position, TURRET_RADIUS))
                        {
                            turrets.erase(turrets.begin() + i);
                            PlaySound(turretDestroySound); 
                            break; 
                        }
                    }
                }
                if (turrets.size() >= MAX_TURRETS)
                {
                    currentState = COMBAT_PHASE;
                    if (currentLevel == 1) current_wave = level1_wave;
                    else if (currentLevel == 2) current_wave = level2_wave;
                    else if (currentLevel == 3) current_wave = level3_wave;
                    enemiesToSpawn = current_wave.size();
                    enemiesSpawned = 0;
                    spawnTimer = 0.0f;
                }
            } 
            break;

            case COMBAT_PHASE:
            { 
                // ---> FIX: Enemy Spawning logic is now inside COMBAT_PHASE
                spawnTimer += dt;
                if (enemiesSpawned < enemiesToSpawn && spawnTimer >= spawnInterval)
                {
                    spawnTimer = 0.0f;
                    Vector2 startPos = TileCenter(waypoints[0].row, waypoints[0].col);
                    EnemyType typeToSpawn = current_wave[enemiesSpawned];
                    enemies.push_back(CreateEnemy(typeToSpawn, startPos, currentLevel));
                    enemiesSpawned++;
                }

                //---> find target to shoot<---
                for (auto& turret : turrets)
                {
                    turret.shootTimer += dt;//--->this increses the shoot timer<---start
                    
                    if (turret.shootTimer >= TURRET_SHOOT_COOLDOWN)//--->when cooldown is over<---
                    {

                        // --->find a target<---
                        if (!enemies.empty())
                        {
                            Enemy* target = &enemies[0];
                            float distance = Vector2Distance(turret.position, target->position);

                            if (distance < TURRET_RANGE)
                            {
                                turret.shootTimer = 0.0f;
                                Bullet newBullet;
                                newBullet.position = turret.position;
                                newBullet.direction = Vector2Normalize(target->position - turret.position);
                                bullets.push_back(newBullet);
								PlaySound(turretShootSound);//---> play sound when turret shoots <---
                            }
                        }
                    }
                }
                // 1) Update bullets
                for (Bullet& bullet : bullets)
                {
                    bullet.position += bullet.direction * BULLET_SPEED * dt;
                    bullet.time += dt;

                    if (bullet.time >= BULLET_LIFE_TIME)
                    {
                        bullet.destroy = true;
                    }
                }
                // ---> collision check <---
                for (auto& bullet : bullets)
                {
                    for (auto& enemy : enemies)
                    {
                        if (CheckCollisionCircles(bullet.position, BULLET_RADIUS, enemy.position, ENEMY_RADIUS))
                        {
                            bullet.destroy = true;
                            enemy.health -= BULLET_DAMAGE;
                            if (enemy.health <= 0)
                            {
                                enemy.shouldBeDestroyed = true;
                                PlaySound(enemyDeathSound);//---> play sound when enemy is destroyed <---
                            }
                        }
                    }
                }

                // 2) Remove bullets
                auto bulletRemoveStart = std::remove_if(bullets.begin(), bullets.end(),
                    [](Bullet& bullet)
                    {
                        return bullet.destroy;
                    });

                bullets.erase(bulletRemoveStart, bullets.end());

                // Hint: You'll need to add a system (remove_if) that flags enemies for deletion then removes them
                // Hint: To handle collision, you'll need a nested for-loop that tests all bullets vs all enemies

                
                // ---> enemy movement <---
                for (auto& enemy : enemies)
                {
                    if (enemy.waypointIndex < waypoints.size() - 1)
                    {
                        Vector2 targetWaypoint = TileCenter(waypoints[enemy.waypointIndex + 1].row, waypoints[enemy.waypointIndex + 1].col);
                        Vector2 direction = Vector2Normalize(targetWaypoint - enemy.position);
                        enemy.position += direction * enemy.speed * dt;

                        if (Vector2Distance(enemy.position, targetWaypoint) < 5.0f)
                        {
                            enemy.position = targetWaypoint;
                            enemy.waypointIndex++;
                        }
                    }
                    else
                    {
                        // --->enemy reached to end and you lost<
                        currentState = LEVEL_LOST;
                    }
                }

                // ---> erase dead enemies <---
                enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                    [](const Enemy& e) { return e.shouldBeDestroyed; }),
                    enemies.end());

                if (enemiesSpawned == enemiesToSpawn && enemies.empty())
                {
                    currentState = LEVEL_WON;
                }
            }
            break;

            case LEVEL_WON:
			{ 
                // If player beats the final level, they win the game.
                if (currentLevel == 3)
                {
					// You Won the Game!
                }
                else
                {
                    // ---> when level is won, press Enter to play again<---
                    if (IsKeyPressed(KEY_ENTER))
                    {
                        currentLevel++;
                        turrets.clear();
                        bullets.clear();
                        enemies.clear();
                        currentState = STRATEGY_PHASE;
                    }
                }
                
            }
            break;

            case LEVEL_LOST:
            { 
                if (IsKeyPressed(KEY_R)) 
                {
                    // reset everything 
                    turrets.clear();
                    bullets.clear();
                    enemies.clear();
                    currentState = STRATEGY_PHASE;
                }
                else if (IsKeyPressed(KEY_S))
                {
                    // reset to level 1
                    currentLevel = 1;
                    turrets.clear();
                    bullets.clear();
                    enemies.clear();
                    currentState = STRATEGY_PHASE;
                }
            } 
            break;
        }
                                                  
        

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw entire grid
        for (int row = 0; row < TILE_COUNT; row++)
        {
            for (int col = 0; col < TILE_COUNT; col++)
            {
                DrawTile(row, col, tiles[row][col]);
            }
        }
        // ---> to draw the turrets <---start
        for (const auto& turret : turrets)
        {
            DrawTexture(turretTexture, turret.position.x - TILE_SIZE / 2, turret.position.y - TILE_SIZE / 2, WHITE);
		}

        for (const Bullet& bullet : bullets)
        {
            DrawTexture(bulletTexture, bullet.position.x - bulletTexture.width / 2, bullet.position.y - bulletTexture.height / 2, WHITE);
        }
        // ---> enemy drawing logicC <---
        
        for (const auto& enemy : enemies)
        {
            Texture2D enemyTexture = enemyNormalTexture;
            if (enemy.type == FAST) enemyTexture = enemyFastTexture;
            else if (enemy.type == HEAVY) enemyTexture = enemyHeavyTexture;

            DrawTexture(enemyTexture, enemy.position.x - TILE_SIZE / 2, enemy.position.y - TILE_SIZE / 2, WHITE);
        }

        
        DrawText(TextFormat("%i", GetFPS()), 760, 10, 20, RED);
        if (currentState == STRATEGY_PHASE)
		{//---> added text to show current level and remaining turrets to place <---
            DrawText(TextFormat("LEVEL %d", currentLevel), 350, 10, 30, WHITE);
            DrawText(TextFormat("Place %d more turrets.", MAX_TURRETS - turrets.size()), 10, 10, 20, WHITE);
            DrawText("Left-Click to build.", 10, 40, 20, WHITE);
        }
        else if (currentState == LEVEL_WON)
        {
            if (currentLevel == 3)
            {
                DrawText("YOU WIN! CONGRATULATIONS!", 200, 350, 30, GREEN);
            }
            else
            {
                DrawText("LEVEL COMPLETE!", 280, 350, 40, GREEN);
                DrawText("Press [ENTER] for the next level.", 240, 400, 20, WHITE);
            }
        }
        else if (currentState == LEVEL_LOST)
        {
            DrawText("LEVEL FAILED!", 300, 350, 40, RED);
            DrawText("Press [R] to Replay or [S] to Start Over.", 180, 400, 20, WHITE);
        }
        else 
        {
            DrawText(TextFormat("Level: %d", currentLevel), 10, 10, 20, WHITE);
            DrawText(TextFormat("Enemies remaining: ~%d", (enemiesToSpawn - enemiesSpawned) + enemies.size()), 10, 40, 20, WHITE);
        }
        EndDrawing();
    }
	// ---> unloading sounds and closing audio device <---
    UnloadSound(turretCreateSound);
    UnloadSound(turretDestroySound);
    UnloadSound(turretShootSound);
    UnloadSound(enemyHitSound);
    UnloadSound(enemyDeathSound);
	// ---> unloading textures <---
    UnloadTexture(turretTexture);
    UnloadTexture(bulletTexture);
    UnloadTexture(enemyNormalTexture);
    UnloadTexture(enemyFastTexture);
    UnloadTexture(enemyHeavyTexture);


    CloseAudioDevice();

    CloseWindow();
    return 0;
}

