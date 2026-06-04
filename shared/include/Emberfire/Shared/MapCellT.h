#pragma once
//
// MapCellT — reconstructed shared base for a single map cell.
// Client: MapCellClient : public MapCellT, ctor passes numLayers.
//
// Holds the collision/walkability flag bitmask. The client casts getCell() results
// to MapCellClient* and calls hasFlag/addFlag/removeFlag/getFlags.
//
#include <vector>

class MapCellT
{
public:
    // Bitmask flags. Values must byte-match the server's .map file persistence
    // (ClientMap writes `(unsigned char)cell->getFlags()` to disk).
    // TODO: confirm exact bit values against server map format.
    enum Flags
    {
        // Bit values derived from real .map flag bytes (fanadin): 0x10 correlates with
        // walls/trees/fences/rocks + void cells (structural blockers); 0x08 with water.
        // The old 0x01/0x02 guesses are never set in shipped maps.
        Unwalkable   = 0x10,  // blocks pathfinding / LOS (walls/trees/void)
        CollideBlock = 0x08,  // water / movement collision
    };

    explicit MapCellT(const int numLayers = 0) : m_numLayers(numLayers) {}
    virtual ~MapCellT() = default;

    int getFlags() const { return m_flags; }

    bool hasFlag(const Flags flag) const { return (m_flags & static_cast<int>(flag)) != 0; }
    void addFlag(const Flags flag) { m_flags |= static_cast<int>(flag); }
    void removeFlag(const Flags flag) { m_flags &= ~static_cast<int>(flag); }

    int getNumLayers() const { return m_numLayers; }

    // Public: ClientMap sets flags on freshly-built MapCellClient objects from
    // outside the cell class, so this can't be protected.
    void setFlags(const int flags) { m_flags = flags; }

    // Per-layer render scale. Stored on the base because both MapCellClient and
    // ClientMap (disk save / cell build) read & write it. TODO confirm semantics.
    float getLayerScale(const int layer) const
    {
        return (layer >= 0 && layer < static_cast<int>(m_layerScale.size()))
                   ? m_layerScale[layer]
                   : 1.f;
    }

    void setLayerScale(const int layer, const float scale)
    {
        if (layer < 0)
            return;
        if (layer >= static_cast<int>(m_layerScale.size()))
            m_layerScale.resize(layer + 1, 1.f);
        m_layerScale[layer] = scale;
    }

private:
    int m_flags{0};
    int m_numLayers{0};
    std::vector<float> m_layerScale;
};
