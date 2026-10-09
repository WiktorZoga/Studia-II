# 02: Shoot a less-protected nearby enemy

Category: enhancement
Status: ready-for-agent
Resolution: completed
Blocked by: 01

## What to build

Each moving agent shoots the less-protected nearby enemy from its new position.
Enemy wetness no longer defines the target for this covers action.

- [x] Evaluate cover from the destination and respect shooting range.
- [x] Restrict selection to the agent's two nearby enemies.
- [x] Break equal post-move protection by original-position protection.
- [x] Verify different targets for the two sides; decorative ties are checked in ticket 03.
