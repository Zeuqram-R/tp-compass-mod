#pragma once

namespace compass {

// Queues the light pillars over the player's pins for this frame (call from a draw hook).
void queuePinBeacons();

// Spawns/updates the sparkle particles around the pins (call once per game frame).
void updateBeaconParticles();

}  // namespace compass
