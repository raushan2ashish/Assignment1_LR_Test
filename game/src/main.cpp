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
constexpr int BULLET_DAMAGE = 50;// ---> damage for each bullet <---

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
    Vector2 position = { 0, 0 };
    int health = 100;
    float speed = 100.0f;
    int waypointIndex = 0;
    bool shouldBeDestroyed = false;
};

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
    // ---> for first enemy <---
    std::vector<Enemy> enemies;
    Enemy firstEnemy;
    firstEnemy.position = TileCenter(waypoints[0].row, waypoints[0].col);
    firstEnemy.speed = 100.0f;
    enemies.push_back(firstEnemy);

    std::vector<Bullet> bullets;
    std::vector<Turret> turrets;// ---> added for turrets <---

    InitWindow(SCREEN_SIZE, SCREEN_SIZE, "Tower Defense");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (turrets.size() < MAX_TURRETS)                //--->added click left mouse to create turret<---start
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
                }
            }
        }                                                
        //---> find target to shoot<---
        for (auto& turret : turrets)
        {
            turret.shootTimer += dt;//--->this increses the shoot timer<---start
            // Check if the cooldown is over
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
                    }
                }
            }
        }
        // 1) Update bullets
        for (Bullet& bullet : bullets)
        {
            bullet.position += bullet.direction * BULLET_SPEED * dt;
            bullet.time += dt;

            bool expired = bullet.time >= BULLET_LIFE_TIME;
            bool collision = false;
            bullet.destroy = expired || collision;
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
                    }
                }
            }
        }
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
        }

        // ---> erase dead enemies <---
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return e.shouldBeDestroyed; }),
            enemies.end());

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
            DrawCircleV(turret.position, TURRET_RADIUS, GRAY);//outer color of turret
			DrawCircleV(turret.position, TURRET_RADIUS - 5, DARKGRAY); //inner color of turret
		}

        for (const Bullet& bullet : bullets)
        {
            DrawCircleV(bullet.position, BULLET_RADIUS, RED);
        }
        // ---> enemy drawing logicC <---
        
        for (const auto& enemy : enemies)
        {
            DrawCircleV(enemy.position, ENEMY_RADIUS, GOLD);
        }

        
        DrawText(TextFormat("%i", GetFPS()), 760, 10, 20, RED);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}

