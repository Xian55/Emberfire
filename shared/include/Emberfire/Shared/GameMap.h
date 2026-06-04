#pragma once
//
// GameMap — reconstructed shared base for a game map.
// Client: ClientMap : public GameMap, ... .
//
// Owns the cell grid, dimensions, name, and the isometric world->screen transform.
// The client casts getCell() results to MapCellClient*.
//
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>

#include "Geo2d.h"
#include "MapCellT.h"

class GameMap
{
public:
    enum Defines
    {
        Layer1 = 0,
        Layer2 = 1,
        Layer3 = 2,
        NumLayers = 3,

        // Base isometric cell footprint in pixels.
        // TODO: confirm exact values against server/client tile art.
        BaseCellWidth  = 64,
        BaseCellHeight = 32,

        // How many base cells make up one terrain tile edge. ClientMap multiplies
        // BaseCellWidth/Height by this to get the terrain tile footprint.
        // MUST equal the loader's cells-per-terrain divisor: loadFromDisk sets
        // terrainWidth = mapWidth/13 (server mapWidth/0xd), so one terrain tile = 13 cells.
        // Was 4 -> terrain tiles placed at 4-cell spacing while representing 13-cell ground,
        // so terrain rendered compressed toward the origin and far cells (SE) showed black.
        TerrainToCellRatio = 13,
    };

    explicit GameMap(const int id = 0) : m_id(id) {}
    virtual ~GameMap() = default;

    int getId() const { return m_id; }

    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    int getMapWidth() const { return m_mapWidth; }
    int getMapHeight() const { return m_mapHeight; }
    int getTerrainWidth() const { return m_terrainWidth; }
    int getTerrainHeight() const { return m_terrainHeight; }

    // Returns the cell at cellId, or nullptr if it does not exist / is empty.
    MapCellT* getCell(const int cellId) const
    {
        auto itr = m_cells.find(cellId);
        return itr == m_cells.end() ? nullptr : itr->second.get();
    }

    // Isometric world-position -> raw (camera-independent) screen position.
    // TODO: real impl must match server/client — exact iso projection constants.
    static Geo2d::Vector2 computeRawScreenPosition(const Geo2d::Vector2& worldPos)
    {
        // Standard 2:1 isometric diamond projection placeholder.
        const float halfW = static_cast<float>(Defines::BaseCellWidth) / 2.f;
        const float halfH = static_cast<float>(Defines::BaseCellHeight) / 2.f;
        return Geo2d::Vector2((worldPos.x - worldPos.y) * halfW,
                              (worldPos.x + worldPos.y) * halfH);
    }

    // -----------------------------------------------------------------------
    // Cell accessors expected by the client's ClientMap / map UI code.
    // These are reconstructed stubs: the geometry below is a placeholder iso
    // projection. TODO: match the real server/client cell<->screen transform.
    // -----------------------------------------------------------------------

    // Cell-id upper-bound guard (== grid size mapWidth*mapHeight). The original client's m_cells was a
    // dense vector of this size, so callers use getNumCells() as the valid-cellId range. m_cells is now
    // a sparse std::map, so this MUST return the grid size — NOT m_cells.size() — or units at high
    // cell ids (south-east) get rejected by ClientMap::assignRenderableIntoCells and never render.
    // For the materialized count, read m_cells.size() directly.
    size_t getNumCells() const { return static_cast<size_t>(m_mapWidth) * static_cast<size_t>(m_mapHeight); }

    // Shared-ptr accessor (the client dynamic_pointer_cast<>s the result to
    // MapCellClient). Default-inserts so the client's m_cells[id] pattern lines up.
    std::shared_ptr<MapCellT> getCellRef(const int cellId)
    {
        return m_cells[cellId];
    }

    // cellId -> raw (camera-independent) screen position.
    Geo2d::Vector2 getCellRenderPos(const int cellId,
                                    const int* customIsleWidth = nullptr,
                                    const int* customCellW = nullptr,
                                    const int* customCellH = nullptr) const
    {
        const int w = customIsleWidth ? *customIsleWidth : (m_mapWidth ? m_mapWidth : 1);
        int cx = w ? (cellId % w) : 0;
        int cy = w ? (cellId / w) : 0;
        return getCellRenderPos(cx, cy, customCellW, customCellH);
    }

    Geo2d::Vector2 getCellRenderPos(const int cellX, const int cellY,
                                    const int* customCellW = nullptr,
                                    const int* customCellH = nullptr) const
    {
        const float halfW = (customCellW ? *customCellW : Defines::BaseCellWidth) / 2.f;
        const float halfH = (customCellH ? *customCellH : Defines::BaseCellHeight) / 2.f;
        return Geo2d::Vector2((cellX - cellY) * halfW, (cellX + cellY) * halfH);
    }

    // Float-coordinate variant used by MapQuester/Minimap (world x,y -> screen).
    Geo2d::Vector2 getCellRenderPosF(const float worldX, const float worldY) const
    {
        const float halfW = Defines::BaseCellWidth / 2.f;
        const float halfH = Defines::BaseCellHeight / 2.f;
        return Geo2d::Vector2((worldX - worldY) * halfW, (worldX + worldY) * halfH);
    }

    // Inverse of getCellRenderPos: screen position -> cellId.
    int getCellId(const Geo2d::Vector2& renderPos,
                  const int* customIsleWidth = nullptr,
                  const int* customCellW = nullptr,
                  const int* customCellH = nullptr) const
    {
        int x = 0, y = 0;
        getCellPosition(x, y, renderPos, customCellW, customCellH);
        const int w = customIsleWidth ? *customIsleWidth : (m_mapWidth ? m_mapWidth : 1);
        return y * w + x;
    }

    // Screen position -> integer cell coordinates.
    void getCellPosition(int& x, int& y, const Geo2d::Vector2& renderPos,
                         const int* customCellW = nullptr,
                         const int* customCellH = nullptr) const
    {
        const float halfW = (customCellW ? *customCellW : Defines::BaseCellWidth) / 2.f;
        const float halfH = (customCellH ? *customCellH : Defines::BaseCellHeight) / 2.f;
        const float fx = (renderPos.x / halfW + renderPos.y / halfH) / 2.f;
        const float fy = (renderPos.y / halfH - renderPos.x / halfW) / 2.f;
        x = static_cast<int>(fx);
        y = static_cast<int>(fy);
    }

    // Screen position -> world (cell-space float) position.
    Geo2d::Vector2 renderPosToWorldPos(const Geo2d::Vector2& renderPos,
                                       const int* customCellW = nullptr,
                                       const int* customCellH = nullptr) const
    {
        const float halfW = (customCellW ? *customCellW : Defines::BaseCellWidth) / 2.f;
        const float halfH = (customCellH ? *customCellH : Defines::BaseCellHeight) / 2.f;
        const float wx = (renderPos.x / halfW + renderPos.y / halfH) / 2.f;
        const float wy = (renderPos.y / halfH - renderPos.x / halfW) / 2.f;
        return Geo2d::Vector2(wx, wy);
    }

    // Loads a map from disk. Reconstructed from server GameMap::loadFromDisk
    // (Ghidra @~0x4b6e?0). FULL on-disk format:
    //   u32 mapWidth (square, <=2000); u32 numCellTextures; cstr[numCellTextures];
    //   u32 numCells; per cell { u32 cellId; u8 flags; <texture-index list>; ... };
    //   u32 numTerrainCells; u32 numTerrainZones; ... (areas)
    // MINIMAL impl: parse the header (dimensions) + drive the ClientMap load hooks so
    // the world enters with a (blank) navigable grid. Cell/terrain texture population
    // is TODO (see build/recon/GHIDRA_WORLD.md). Movement/objects/combat are
    // server-authoritative, so a blank map is still fully playable.
    // Full .map parser. Format reversed from the client GameMap::loadFromDisk
    // (decompile FUN_00555fc0) and byte-validated against real maps (e.g. fanadin.map):
    //   u32 width(<=2000); u32 numCellTex; numCellTex x cstr;
    //   u32 numCells; per cell { u32 cellId; u8 flags; 3x layer{ u8 present; if present: u32 texIdx; f32 scale } };
    //   u32 numTerrainTex; numTerrainTex x cstr; u32 numTerrainCells; per { u32 terrainCellId; u32 terrainTexIdx };
    //   (trailing zone/area sections are non-render and ignored).
    // Every read is bounds-checked: any short/inconsistent buffer aborts to false
    // (blank-but-playable, no crash) rather than feeding bad indices to the hooks.
    virtual bool loadFromDisk(const std::string& name)
    {
        std::ifstream f("maps/" + name + ".map", std::ios::binary);
        if (!f.is_open())
            return false;
        const std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(f)),
                                             std::istreambuf_iterator<char>());

        std::size_t o = 0;
        bool ok = true;
        auto rdU32 = [&](std::uint32_t& v) {
            if (o + 4 > buf.size()) { ok = false; return; }
            v = std::uint32_t(buf[o]) | (std::uint32_t(buf[o+1])<<8) |
                (std::uint32_t(buf[o+2])<<16) | (std::uint32_t(buf[o+3])<<24);
            o += 4;
        };
        auto rdU8 = [&](std::uint8_t& v) {
            if (o + 1 > buf.size()) { ok = false; return; }
            v = buf[o++];
        };
        auto rdF32 = [&](float& v) {
            if (o + 4 > buf.size()) { ok = false; return; }
            std::uint32_t bits = std::uint32_t(buf[o]) | (std::uint32_t(buf[o+1])<<8) |
                                 (std::uint32_t(buf[o+2])<<16) | (std::uint32_t(buf[o+3])<<24);
            std::memcpy(&v, &bits, 4);
            o += 4;
        };
        auto rdStr = [&](std::string& s) {
            const std::size_t start = o;
            while (o < buf.size() && buf[o] != 0) ++o;
            if (o >= buf.size()) { ok = false; return; }
            s.assign(reinterpret_cast<const char*>(&buf[start]), o - start);
            ++o; // skip NUL
        };

        std::uint32_t width = 0; rdU32(width);
        if (!ok || width == 0 || width > 2000)
            return false;

        std::uint32_t numCellTex = 0; rdU32(numCellTex);
        if (!ok || numCellTex > 1000000u)
            return false;
        std::vector<std::string> cellTex(numCellTex);
        for (std::uint32_t i = 0; i < numCellTex && ok; ++i) rdStr(cellTex[i]);
        if (!ok)
            return false;

        const int W = int(width);
        const long long cellCount = (long long)W * (long long)W;

        setName(name);
        startedLoading();              // ClientMap: clear render state, stop calc threads
        setMapWidth(W);
        setMapHeight(W);               // square grid
        const int terrainW = W / 13;   // server: mapWidth / 0xd
        setTerrainWidth(terrainW);
        setTerrainHeight(terrainW);
        onResize();

        // ---- cells ----
        std::uint32_t numCells = 0; rdU32(numCells);
        for (std::uint32_t c = 0; c < numCells && ok; ++c)
        {
            std::uint32_t cellId = 0; std::uint8_t flags = 0;
            rdU32(cellId); rdU8(flags);
            if (!ok) break;
            if ((long long)cellId >= cellCount) { ok = false; break; }

            std::vector<std::shared_ptr<std::string>> textures(Defines::NumLayers);
            std::vector<float> layerScale(Defines::NumLayers, 0.f);
            for (int L = 0; L < Defines::NumLayers && ok; ++L)
            {
                std::uint8_t present = 0; rdU8(present);
                if (!ok) break;
                if (present)
                {
                    std::uint32_t idx = 0; float sc = 1.f;
                    rdU32(idx); rdF32(sc);
                    if (!ok) break;
                    if (idx >= numCellTex) { ok = false; break; }
                    textures[L] = std::make_shared<std::string>(cellTex[idx]);
                    layerScale[L] = sc;
                }
            }
            if (!ok) break;
            onCellDataLoaded(int(cellId), int(flags), textures, layerScale);
        }
        if (!ok) { finishedLoading(); return false; }

        onFinishedLoadingCells();      // re-sizes terrain buffers; must precede terrain loads

        // ---- terrain palette + per-terrain-cell assignment ----
        std::uint32_t numTerrainTex = 0; rdU32(numTerrainTex);
        if (ok && numTerrainTex <= 100000u)
        {
            std::vector<std::string> terrTex(numTerrainTex);
            for (std::uint32_t i = 0; i < numTerrainTex && ok; ++i) rdStr(terrTex[i]);

            std::uint32_t numTerrainCells = 0; rdU32(numTerrainCells);
            const long long terrCount = (long long)terrainW * (long long)terrainW;

            // DEFAULT-FILL: the .map only stores terrain tiles that differ from the base
            // (e.g. fanadin.map lists 2122 of 2809). Unlisted tiles must fall back to the
            // base terrain texture, else they render as black holes in the ground.
            // The base is terrain palette index 0 (the first/most-common ground texture).
            if (ok && numTerrainTex > 0 && !terrTex[0].empty())
                for (long long t = 0; t < terrCount; ++t)
                    onTerrainTextureLoaded(int(t), terrTex[0]);

            for (std::uint32_t i = 0; i < numTerrainCells && ok; ++i)
            {
                std::uint32_t tId = 0, tIdx = 0;
                rdU32(tId); rdU32(tIdx);
                if (!ok) break;
                if ((long long)tId < terrCount && tIdx < numTerrainTex)
                    onTerrainTextureLoaded(int(tId), terrTex[tIdx]); // override with the explicit tile
            }

            // ---- terrain zones, then areas (mirror of ClientMap::saveToDisk) ----
            // Each section: u32 count, then count x { u32 terrainCellId, u32 id }.
            // Drives the zone/area-entry name notifications (ZoneNotification).
            std::uint32_t numZones = 0; rdU32(numZones);
            for (std::uint32_t i = 0; i < numZones && ok; ++i)
            {
                std::uint32_t tId = 0, zoneId = 0;
                rdU32(tId); rdU32(zoneId);
                if (!ok) break;
                if ((long long)tId < terrCount)
                    onTerrainZoneLoaded(int(tId), int(zoneId));
            }

            std::uint32_t numAreas = 0; rdU32(numAreas);
            for (std::uint32_t i = 0; i < numAreas && ok; ++i)
            {
                std::uint32_t tId = 0, areaId = 0;
                rdU32(tId); rdU32(areaId);
                if (!ok) break;
                if ((long long)tId < terrCount)
                    onTerrainAreaLoaded(int(tId), int(areaId));
            }
        }

        finishedLoading();             // ClientMap: restart render-calc threads
        return true;
    }

    // World position -> terrain-cell index (row-major ty*terrainW + tx over the terrain
    // grid). This is how the .map keys terrain textures + zones/areas. Use this for zone
    // lookups instead of the screen-space iso inverse (renderPosToTerrainIdRelative),
    // whose inverse projection is currently imprecise and lands on the wrong terrain cell.
    int worldPosToTerrainId(const Geo2d::Vector2& w) const
    {
        if (m_terrainWidth <= 0 || m_mapWidth <= 0)
            return 0;
        const int tx = int(w.x * float(m_terrainWidth) / float(m_mapWidth));
        const int ty = int(w.y * float(m_terrainWidth) / float(m_mapWidth));
        if (tx < 0 || ty < 0 || tx >= m_terrainWidth || ty >= m_terrainWidth)
            return 0;
        return ty * m_terrainWidth + tx;
    }

    // Resizes the (square) cell grid to size*size. Stub. TODO: real resize semantics.
    virtual void resize(const int /*size*/) {}

protected:
    void setMapWidth(const int w) { m_mapWidth = w; }
    void setMapHeight(const int h) { m_mapHeight = h; }
    void setTerrainWidth(const int w) { m_terrainWidth = w; }
    void setTerrainHeight(const int h) { m_terrainHeight = h; }

    // Ensures a cell object exists for cellId and returns it.
    MapCellT* ensureCell(const int cellId)
    {
        auto& slot = m_cells[cellId];
        if (!slot)
            slot = std::make_shared<MapCellT>(Defines::NumLayers);
        return slot.get();
    }

    // ---- virtual load hooks (overridden by ClientMap) ----
    // Base bodies are no-ops; the real loader (.map parse + async cell streaming)
    // lives in the client. TODO: server-side map loading must match the .map format.
    virtual void startedLoading() {}
    virtual void finishedLoading() {}
    virtual void onFinishedLoadingCells() {}
    virtual void onResize() {}
    virtual void onTerrainTextureLoaded(const int /*terrainId*/, const std::string& /*texture*/) {}
    virtual void onTerrainZoneLoaded(const int /*terrainId*/, const int /*zoneId*/) {}
    virtual void onTerrainAreaLoaded(const int /*terrainId*/, const int /*areaId*/) {}
    virtual void onCellDataLoaded(const int /*cellId*/, const int /*flags*/,
                                  const std::vector<std::shared_ptr<std::string>>& /*textures*/,
                                  const std::vector<float>& /*layerScale*/) {}

protected:
    // cellId -> cell. Sparse (only non-empty cells materialized).
    // Protected: ClientMap indexes m_cells directly (m_cells[id] = ...).
    std::map<int, std::shared_ptr<MapCellT>> m_cells;

private:
    int m_id{0};
    int m_mapWidth{0};
    int m_mapHeight{0};
    int m_terrainWidth{0};
    int m_terrainHeight{0};

    std::string m_name;
};
