#pragma once
//
// MapLogic — reconstructed shared static pathfinding / line-of-sight helpers.
// All members are static; the client calls them as MapLogic::xxx(...).
//
// Observed call sites (client):
//   MapLogic::getIndexesAround(vector<int>&, centerCell, width, range)
//   MapLogic::checkLosToC(map, fromWorld, toWorld, MapCellT::Unwalkable)
//   MapLogic::constructPathTo(map, outPath, fromWorld, toWorld)
//   MapLogic::getPathLength(start, path)
//
#include <vector>
#include <cmath>
#include <queue>
#include <algorithm>
#include <unordered_map>

#include "Geo2d.h"
#include "GameMap.h"
#include "MapCellT.h"

class MapLogic
{
private:
    // Rockwall marker = Unwalkable(0x10) AND 0x20 both set (== 0x30). Bare 0x10 alone sits on
    // walkable dirt/leaf cells, so it must NOT block by itself.
    static constexpr int WALL_MASK = MapCellT::Unwalkable | 0x20; // 0x30

    // Is the cell at integer (cx,cy) impassable? Data-backed from fanadin.map flag<->texture
    // correlation: only water (CollideBlock 0x08) and rockwall (flags & 0x30 == 0x30) block.
    // The old mask (Unwalkable|CollideBlock = 0x18) wrongly blocked the 1197 bare-0x10 dirt/leaf
    // cells, so client LOS/WASD refused valid moves (invisible walls the steam client lacks).
    // Out-of-bounds counts as blocked; unmaterialized (null) cells are walkable (server corrects).
    static bool cellBlocked(const GameMap& map, const int cx, const int cy)
    {
        if (cx < 0 || cy < 0 || cx >= map.getMapWidth() || cy >= map.getMapHeight())
            return true;
        const MapCellT* c = map.getCell(cy * map.getMapWidth() + cx);
        if (c == nullptr)
            return false;
        const int f = c->getFlags();
        return (f & MapCellT::CollideBlock) != 0 || (f & WALL_MASK) == WALL_MASK;
    }

public:
    // Fills `out` with cell indexes in a (2*range+1) square block centered on
    // centerCell, given a row-major grid of the given width. Out-of-range indexes
    // are clamped/skipped. This one is fully determinable from grid geometry.
    static void getIndexesAround(std::vector<int>& out, const int centerCell,
                                 const int width, const int range)
    {
        out.clear();
        if (width <= 0)
            return;

        const int cx = centerCell % width;
        const int cy = centerCell / width;

        for (int dy = -range; dy <= range; ++dy)
        {
            const int y = cy + dy;
            if (y < 0)
                continue;
            for (int dx = -range; dx <= range; ++dx)
            {
                const int x = cx + dx;
                if (x < 0 || x >= width)
                    continue;
                out.push_back(y * width + x);
            }
        }
    }

    // Line-of-sight test from `from` to `to` in world coordinates, treating cells
    // carrying `blockFlag` (e.g. MapCellT::Unwalkable) as opaque blockers.
    // TODO: real impl must match server — proper grid raycast (e.g. Bresenham/DDA
    // over cell ids) querying GameMap::getCell()->hasFlag(blockFlag).
    static bool checkLosToC(const GameMap& map, const Geo2d::Vector2& from,
                            const Geo2d::Vector2& to, const int /*blockFlag*/)
    {
        // Sample the straight segment at sub-cell steps; blocked if any cell is impassable.
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(dist / 0.25f));
        for (int i = 0; i <= steps; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const int cx = static_cast<int>(std::floor(from.x + dx * t));
            const int cy = static_cast<int>(std::floor(from.y + dy * t));
            if (cellBlocked(map, cx, cy))
                return false;
        }
        return true;
    }

    // Builds a path of world-space waypoints from `from` to `to`, appending into
    // `outPath`. Empty result == no path found.
    // TODO: real impl must match server — A* over the cell grid using
    // MapCellT::Unwalkable, then waypoint simplification.
    static void constructPathTo(const GameMap& map, std::vector<Geo2d::Vector2>& outPath,
                                const Geo2d::Vector2& from, const Geo2d::Vector2& to)
    {
        outPath.clear();

        const int W = map.getMapWidth();
        const int H = map.getMapHeight();
        if (W <= 0 || H <= 0) { outPath.push_back(to); return; }

        const int sx = static_cast<int>(std::floor(from.x));
        const int sy = static_cast<int>(std::floor(from.y));
        const int tx = static_cast<int>(std::floor(to.x));
        const int ty = static_cast<int>(std::floor(to.y));

        if (sx == tx && sy == ty) { outPath.push_back(to); return; }
        if (cellBlocked(map, tx, ty)) return;             // destination impassable -> no path

        const int startId = sy * W + sx;
        const int destId  = ty * W + tx;

        std::unordered_map<int, float> gScore;            // best cost to reach a cell
        std::unordered_map<int, int>   cameFrom;
        // (f, cellId) min-heap
        using Node = std::pair<float, int>;
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;

        auto heuristic = [&](int x, int y) {
            const float hx = static_cast<float>(tx - x);
            const float hy = static_cast<float>(ty - y);
            return std::sqrt(hx * hx + hy * hy);
        };

        gScore[startId] = 0.f;
        open.push({ heuristic(sx, sy), startId });

        static const int DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
        static const int DY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

        const int maxNodes = 40000;                       // bound the search on big maps
        int processed = 0;
        bool found = false;

        while (!open.empty() && processed < maxNodes)
        {
            const Node top = open.top();
            open.pop();
            const int cur = top.second;
            if (cur == destId) { found = true; break; }
            ++processed;

            const int cx = cur % W;
            const int cy = cur / W;
            const float cg = gScore[cur];

            for (int k = 0; k < 8; ++k)
            {
                const int nx = cx + DX[k];
                const int ny = cy + DY[k];
                if (cellBlocked(map, nx, ny))
                    continue;
                // Don't cut diagonal corners between two blocked orthogonals.
                if (k >= 4 && (cellBlocked(map, cx + DX[k], cy) || cellBlocked(map, cx, cy + DY[k])))
                    continue;

                const float step = (k >= 4) ? 1.41421356f : 1.f;
                const int nid = ny * W + nx;
                const float ng = cg + step;
                auto it = gScore.find(nid);
                if (it == gScore.end() || ng < it->second)
                {
                    gScore[nid] = ng;
                    cameFrom[nid] = cur;
                    open.push({ ng + heuristic(nx, ny), nid });
                }
            }
        }

        if (!found) return;                               // no path -> empty (caller won't move)

        // Reconstruct cell-center waypoints, dest -> start, then reverse.
        std::vector<Geo2d::Vector2> rev;
        int cur = destId;
        while (cur != startId)
        {
            const int cx = cur % W;
            const int cy = cur / W;
            rev.push_back(Geo2d::Vector2(cx + 0.5f, cy + 0.5f));
            auto it = cameFrom.find(cur);
            if (it == cameFrom.end()) break;
            cur = it->second;
        }
        for (auto it = rev.rbegin(); it != rev.rend(); ++it)
            outPath.push_back(*it);

        if (!outPath.empty())
            outPath.back() = to;                          // end exactly on the clicked point
    }

    // Total euclidean length of walking `path` starting from `start`.
    static float getPathLength(const Geo2d::Vector2& start,
                               const std::vector<Geo2d::Vector2>& path)
    {
        float total = 0.f;
        Geo2d::Vector2 prev = start;
        for (const auto& p : path)
        {
            total += prev.getDist(p);
            prev = p;
        }
        return total;
    }
};
