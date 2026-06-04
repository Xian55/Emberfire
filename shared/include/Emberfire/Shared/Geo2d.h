#pragma once
// Forwarder: the CANONICAL Geo2d lives in the client's parent dir (../Geo2d.h). The Shared/ dir is
// reached via the client/Shared junction, so from a Shared header "../Geo2d.h" resolves to client/Geo2d.h.
// This avoids a second Geo2d::Vector2 definition (was: a placeholder here clashing with the real one).
#include "../Geo2d.h"
