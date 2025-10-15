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

constexpr float TURRET_RADIUS = TILE_SIZE * 1.4f;// ---> added radius for turrets <---
constexpr float TURRET_RANGE = 200.0f; //---> this defines the ranfe of the turrets <---
constexpr float TURRET_SHOOT_COOLDOWN = 0.5f; //---> cooldown time for the turrets <---
constexpr int MAX_TURRETS = 5;                   // ---> added number of maximum turrets <---

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
    int curr = 0;
    int next = curr + 1;

    Vector2 enemyPosition = TileCenter(waypoints[curr].row, waypoints[curr].col);
    float enemySpeed = 250.0f;
    float minDistance = enemySpeed / 60.0f;
    minDistance *= 1.1f;
    bool atEnd = false;

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
                
                float distance = Vector2Distance(turret.position, enemyPosition);//--->to check distance b/w turret and enemy<---
 
                if (distance < TURRET_RANGE)//--->shoot when enemy is in range<---
                {
                    
                    turret.shootTimer = 0.0f;

                    //--->to create new bullet<---
                    Bullet newBullet;
                    newBullet.position = turret.position;
                    newBullet.direction = Vector2Normalize(enemyPosition - turret.position);
                    bullets.push_back(newBullet);
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

        if (!atEnd)
        {
            Vector2 from = TileCenter(waypoints[curr].row, waypoints[curr].col);
            Vector2 to = TileCenter(waypoints[next].row, waypoints[next].col);
            Vector2 direction = Vector2Normalize(to - from);
            enemyPosition += direction * enemySpeed * dt;

            if (CheckCollisionPointCircle(enemyPosition, to, minDistance))
            {
                enemyPosition = to;

                curr++;
                next++;
                atEnd = curr == waypoints.size() - 1;
            }
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
            DrawCircleV(turret.position, TURRET_RADIUS, GRAY);//outer color of turret
			DrawCircleV(turret.position, TURRET_RADIUS - 5, DARKGRAY); //inner color of turret
		}// ---> end <---

        for (const Bullet& bullet : bullets)
        {
            DrawCircleV(bullet.position, BULLET_RADIUS, RED);
        }


        DrawCircleV(enemyPosition, 20.0f, GOLD);
        DrawText(TextFormat("%i", GetFPS()), 760, 10, 20, RED);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}

//std::vector<int> numbers{ 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
//for (int i = 0; i < numbers.size(); i++)
//{
//    if (numbers[i] % 2 == 1)
//    {
//        numbers.erase(numbers.begin() + i);
//        i--;
//    }
//}
//
// 1, 1, 2, 2, 3, 3, 4, 4, 5, 5
// 2, 2, 4, 4, 1, 1, 3, 3, 5, 5,
//auto removeStart = std::remove_if(numbers.begin(), numbers.end(),
//    [](int n)
//    {
//        // Return true if element should be removed!
//        return n % 2 == 1;
//    });
//
//// Note that remove_if only sorts the data. You must call erase separately to remove everything!
//numbers.erase(removeStart, numbers.end());

//for (int row = 0; row < TILE_COUNT; row++)
//{
//    float y = row * TILE_SIZE;
//    DrawLineEx({ 0.0f, y }, { SCREEN_SIZE, y }, 2.0f, LIGHTGRAY);
//}
//
//for (int col = 0; col < TILE_COUNT; col++)
//{
//    float x = col * TILE_SIZE;
//    DrawLineEx({ x, 0.0f }, { x, SCREEN_SIZE }, 2.0f, LIGHTGRAY);
//}