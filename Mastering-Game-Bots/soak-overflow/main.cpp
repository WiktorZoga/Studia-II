#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

struct AgentDefinition {
    int id{};
    int player_id{};
    int shoot_cooldown{};
    int optimal_range{};
    int soaking_power{};
    int initial_splash_bombs{};
};

struct Tile {
    int x{};
    int y{};
    int type{};
};

struct Position {
    int x{};
    int y{};
};

struct GameSetup {
    int my_id{};
    int width{};
    int height{};
    std::vector<AgentDefinition> agents;
    std::vector<Tile> tiles;
};

const Tile* tile_at(const GameSetup& setup, Position position) {
    if (position.x < 0 || position.y < 0
        || position.x >= setup.width || position.y >= setup.height) {
        return nullptr;
    }
    const auto tile = std::ranges::find_if(setup.tiles, [position](const Tile& tile) {
        return tile.x == position.x && tile.y == position.y;
    });
    return tile == setup.tiles.end() ? nullptr : &*tile;
}

int cover_reduction(const GameSetup& setup, Position shooter, Position target) {
    int reduction = 0;
    const auto sign = [](int value) { return (value > 0) - (value < 0); };
    for (const auto direction : {Position{shooter.x - target.x, 0},
                                 Position{0, shooter.y - target.y}}) {
        if (std::abs(direction.x) <= 1 && std::abs(direction.y) <= 1) {
            continue;
        }
        const Position cover{target.x + sign(direction.x),
                             target.y + sign(direction.y)};
        // The referee excludes orthogonal and diagonal shooter adjacency.
        if (std::max(std::abs(shooter.x - cover.x),
                     std::abs(shooter.y - cover.y)) <= 1) {
            continue;
        }
        const auto* tile = tile_at(setup, cover);
        if (tile != nullptr && tile->type == 1) {
            reduction = std::max(reduction, 50);
        } else if (tile != nullptr && tile->type == 2) {
            reduction = std::max(reduction, 75);
        }
    }
    return reduction;
}

struct AgentState {
    int id{};
    int x{};
    int y{};
    int cooldown{};
    int splash_bombs{};
    int wetness{};
};

struct TurnState {
    std::vector<AgentState> agents;
    int my_agent_count{};
};

bool read_setup(std::istream& input, GameSetup& setup) {
    int agent_count{};
    if (!(input >> setup.my_id >> agent_count) || agent_count < 0) {
        return false;
    }

    setup.agents.resize(agent_count);
    for (auto& agent : setup.agents) {
        if (!(input >> agent.id >> agent.player_id >> agent.shoot_cooldown
                    >> agent.optimal_range >> agent.soaking_power
                    >> agent.initial_splash_bombs)) {
            return false;
        }
    }

    if (!(input >> setup.width >> setup.height)
        || setup.width <= 0 || setup.height <= 0) {
        return false;
    }

    for (int y = 0; y < setup.height; ++y) {
        for (int x = 0; x < setup.width; ++x) {
            Tile tile;
            if (!(input >> tile.x >> tile.y >> tile.type)) {
                return false;
            }
            setup.tiles.push_back(tile);
        }
    }
    return true;
}

bool read_turn(std::istream& input, int agent_count, TurnState& turn) {
    if (agent_count < 0) {
        return false;
    }
    turn.agents.resize(agent_count);
    for (auto& agent : turn.agents) {
        if (!(input >> agent.id >> agent.x >> agent.y >> agent.cooldown
                    >> agent.splash_bombs >> agent.wetness)) {
            return false;
        }
    }
    return static_cast<bool>(input >> turn.my_agent_count)
        && turn.my_agent_count >= 0;
}

std::vector<const AgentState*> surviving_enemies(
    const GameSetup& setup, const TurnState& turn) {
    std::vector<const AgentState*> enemies;
    for (const auto& definition : setup.agents) {
        if (definition.player_id == setup.my_id) {
            continue;
        }
        const auto enemy = std::ranges::find(
            turn.agents, definition.id, &AgentState::id);
        if (enemy != turn.agents.end()) {
            enemies.push_back(&*enemy);
        }
    }
    return enemies;
}

const AgentState* wettest_enemy(const GameSetup& setup, const TurnState& turn) {
    const AgentState* target = nullptr;
    // Preserve initial enemy order when wetness is tied, as in the referee.
    for (const auto* enemy : surviving_enemies(setup, turn)) {
        if (target == nullptr || enemy->wetness > target->wetness) {
            target = enemy;
        }
    }
    return target;
}

std::vector<const AgentState*> nearby_enemies(
    const GameSetup& setup, const TurnState& turn, Position position) {
    auto enemies = surviving_enemies(setup, turn);
    const auto distance = [position](const AgentState* enemy) {
        return std::abs(position.x - enemy->x) + std::abs(position.y - enemy->y);
    };
    std::stable_sort(enemies.begin(), enemies.end(), [&](const auto* a, const auto* b) {
        return distance(a) < distance(b);
    });
    if (enemies.size() > 2) {
        enemies.resize(2);
    }
    return enemies;
}

std::optional<Position> best_cover_move(
    const GameSetup& setup, const TurnState& turn, const AgentState& agent) {
    const Position current{agent.x, agent.y};
    const auto enemies = nearby_enemies(setup, turn, current);
    if (enemies.empty()) {
        return std::nullopt;
    }
    const auto protection = [&](Position position) {
        int reduction = 100;
        for (const auto* enemy : enemies) {
            reduction = std::min(reduction,
                cover_reduction(setup, {enemy->x, enemy->y}, position));
        }
        return reduction;
    };
    int best_protection = protection(current);
    std::optional<Position> destination;
    for (const auto position : {Position{current.x, current.y - 1},
                                Position{current.x, current.y + 1},
                                Position{current.x - 1, current.y},
                                Position{current.x + 1, current.y}}) {
        const auto* tile = tile_at(setup, position);
        if (tile == nullptr || tile->type != 0) {
            continue;
        }
        const bool occupied = std::ranges::any_of(turn.agents, [position](const auto& other) {
            return other.x == position.x && other.y == position.y;
        });
        if (occupied) {
            continue;
        }
        const int candidate_protection = protection(position);
        if (candidate_protection > best_protection) {
            best_protection = candidate_protection;
            destination = position;
        }
    }
    return destination;
}

const AgentState* least_protected_enemy(
    const GameSetup& setup, const TurnState& turn,
    const AgentState& shooter, Position destination) {
    const auto definition = std::ranges::find(setup.agents, shooter.id, &AgentDefinition::id);
    if (definition == setup.agents.end()) {
        return nullptr;
    }
    const AgentState* target = nullptr;
    int best_protection = 101;
    int best_original_protection = 101;
    for (const auto* enemy : nearby_enemies(setup, turn, destination)) {
        const Position enemy_position{enemy->x, enemy->y};
        const int distance = std::abs(destination.x - enemy->x)
                           + std::abs(destination.y - enemy->y);
        if (distance > 2 * definition->optimal_range) {
            continue;
        }
        const int protection = cover_reduction(setup, destination, enemy_position);
        const int original_protection = cover_reduction(
            setup, {shooter.x, shooter.y}, enemy_position);
        if (protection < best_protection
            || (protection == best_protection
                && original_protection < best_original_protection)) {
            target = enemy;
            best_protection = protection;
            best_original_protection = original_protection;
        }
    }
    return target;
}

bool in_blast(Position center, Position position) {
    return std::max(std::abs(center.x - position.x),
                    std::abs(center.y - position.y)) <= 1;
}

int manhattan(Position a, Position b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

std::array<Position, 4> adjacent(Position position) {
    return {{{position.x + 1, position.y}, {position.x - 1, position.y},
             {position.x, position.y + 1}, {position.x, position.y - 1}}};
}

struct BombAction {
    Position destination;
    std::optional<Position> aim;
};

std::vector<BombAction> plan_bomb_actions(
    const GameSetup& setup, const TurnState& turn,
    const std::vector<const AgentState*>& owned) {
    const int cells = setup.width * setup.height;
    const auto index = [&](Position p) { return p.y * setup.width + p.x; };
    const auto position = [&](int i) { return Position{i % setup.width, i / setup.width}; };
    const auto inside = [&](Position p) {
        return p.x >= 0 && p.y >= 0 && p.x < setup.width && p.y < setup.height;
    };
    std::vector<int> terrain(cells, 2), region(cells, -1);
    for (const auto& tile : setup.tiles) {
        terrain[index({tile.x, tile.y})] = tile.type;
    }

    // Flood-fill floor areas through agent cells to identify enclosed bunkers.
    int region_count = 0;
    for (int start = 0; start < cells; ++start) {
        if (terrain[start] != 0 || region[start] != -1) {
            continue;
        }
        std::vector<int> queue{start};
        region[start] = region_count;
        for (std::size_t head = 0; head < queue.size(); ++head) {
            for (const auto next : adjacent(position(queue[head]))) {
                if (inside(next) && terrain[index(next)] == 0 && region[index(next)] == -1) {
                    region[index(next)] = region_count;
                    queue.push_back(index(next));
                }
            }
        }
        ++region_count;
    }
    std::vector<bool> protected_region(region_count, false), claimed(region_count, false);
    std::vector<BombAction> actions;
    std::vector<std::size_t> order;
    for (std::size_t i = 0; i < owned.size(); ++i) {
        const Position current{owned[i]->x, owned[i]->y};
        actions.push_back({current, std::nullopt});
        order.push_back(i);
        const int area = region[index(current)];
        if (area >= 0) {
            protected_region[area] = true;
        }
    }
    std::vector<std::vector<const AgentState*>> groups(region_count);
    for (const auto* enemy : surviving_enemies(setup, turn)) {
        const int area = region[index({enemy->x, enemy->y})];
        if (area >= 0 && !protected_region[area]) {
            groups[area].push_back(enemy);
        }
    }
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) {
        return owned[a]->splash_bombs > owned[b]->splash_bombs;
    });

    for (const auto actor : order) {
        if (owned[actor]->splash_bombs <= 0) {
            continue;
        }
        std::vector<bool> blocked(cells, false);
        for (const auto& agent : turn.agents) {
            blocked[index({agent.x, agent.y})] = true;
        }
        for (const auto& action : actions) {
            blocked[index(action.destination)] = true;
            if (action.aim) {
                for (int cell = 0; cell < cells; ++cell) {
                    if (in_blast(*action.aim, position(cell))) {
                        blocked[cell] = true;
                    }
                }
            }
        }
        const int start = index(actions[actor].destination);
        std::vector<int> distance(cells, -1), first_step(cells, start), queue{start};
        distance[start] = 0;
        for (std::size_t head = 0; head < queue.size(); ++head) {
            const int current = queue[head];
            for (const auto next : adjacent(position(current))) {
                if (!inside(next)) {
                    continue;
                }
                const int cell = index(next);
                if (terrain[cell] == 0 && !blocked[cell] && distance[cell] == -1) {
                    distance[cell] = distance[current] + 1;
                    first_step[cell] = current == start ? cell : first_step[current];
                    queue.push_back(cell);
                }
            }
        }

        int best_steps = cells + 1, best_area = -1, firing_cell = start;
        Position best_aim{};
        for (int area = 0; area < region_count; ++area) {
            if (claimed[area] || groups[area].empty()) {
                continue;
            }
            int needed = 0;
            for (const auto* enemy : groups[area]) {
                needed = std::max(needed, (100 - enemy->wetness + 29) / 30);
            }
            if (needed > owned[actor]->splash_bombs) {
                continue;
            }
            for (int aim_cell = 0; aim_cell < cells; ++aim_cell) {
                const Position aim = position(aim_cell);
                if (!std::ranges::all_of(groups[area], [aim](const auto* enemy) {
                        return in_blast(aim, {enemy->x, enemy->y});
                    })) {
                    continue;
                }
                const bool ally_nearby = std::ranges::any_of(order, [&](auto other) {
                    return other != actor && in_blast(aim, actions[other].destination);
                });
                if (ally_nearby) {
                    continue;
                }
                for (const int cell : queue) {
                    const Position firing = position(cell);
                    if (distance[cell] < best_steps && manhattan(firing, aim) <= 4
                        && !in_blast(aim, firing)) {
                        best_steps = distance[cell];
                        best_area = area;
                        firing_cell = cell;
                        best_aim = aim;
                    }
                }
            }
        }
        if (best_area >= 0) {
            claimed[best_area] = true;
            actions[actor].destination = position(first_step[firing_cell]);
            if (best_steps <= 1) {
                actions[actor].aim = best_aim;
            }
        }
    }
    return actions;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    GameSetup setup;
    if (!read_setup(std::cin, setup)) {
        std::cerr << "Incomplete or invalid game setup\n";
        return 1;
    }
    // The bomb tutorial supplies bombs; earlier tutorial setups supply none.
    const bool bomb_tutorial = std::ranges::any_of(setup.agents, [&](const auto& agent) {
        return agent.player_id == setup.my_id && agent.initial_splash_bombs > 0;
    });

    std::cerr << "Player " << setup.my_id << ", map " << setup.width << 'x'
              << setup.height << ", tiles " << setup.tiles.size() << '\n';

    int turn_number{};
    int agent_count{};
    while (std::cin >> agent_count) {
        TurnState turn;
        if (!read_turn(std::cin, agent_count, turn)) {
            std::cerr << "Incomplete or invalid turn\n";
            return 1;
        }

        std::vector<const AgentState*> owned_agents;
        for (const auto& agent : turn.agents) {
            // IDs link the permanent definitions to this turn's state.
            const auto definition = std::ranges::find(
                setup.agents, agent.id, &AgentDefinition::id);
            if (definition == setup.agents.end()) {
                std::cerr << "Unknown agent ID: " << agent.id << '\n';
                return 1;
            }
            if (definition->player_id == setup.my_id) {
                owned_agents.push_back(&agent);
                std::cerr << "Turn " << turn_number << ": owned agent "
                          << agent.id << " at (" << agent.x << ',' << agent.y
                          << "), cooldown " << agent.cooldown << ", bombs "
                          << agent.splash_bombs << ", wetness " << agent.wetness
                          << '\n';
            }
        }
        if (owned_agents.size() != static_cast<std::size_t>(turn.my_agent_count)) {
            std::cerr << "Owned agent count does not match the input\n";
            return 1;
        }
        if (bomb_tutorial) {
            const auto actions = plan_bomb_actions(setup, turn, owned_agents);
            for (std::size_t i = 0; i < owned_agents.size(); ++i) {
                const auto* agent = owned_agents[i];
                const auto& action = actions[i];
                const bool moved = action.destination.x != agent->x || action.destination.y != agent->y;
                std::cout << agent->id << ';';
                if (moved) {
                    std::cout << "MOVE " << action.destination.x << ' ' << action.destination.y;
                }
                if (action.aim) {
                    if (moved) {
                        std::cout << ';';
                    }
                    std::cout << "THROW " << action.aim->x << ' ' << action.aim->y;
                } else if (!moved) {
                    std::cout << "MESSAGE waiting";
                }
                std::cout << '\n';
            }
            std::cout.flush();
            ++turn_number;
            continue;
        }
        const auto* fallback_target = wettest_enemy(setup, turn);
        for (const auto* agent : owned_agents) {
            const auto destination = best_cover_move(setup, turn, *agent);
            const auto* target = destination
                ? least_protected_enemy(setup, turn, *agent, *destination)
                : fallback_target;
            std::cout << agent->id << ';';
            if (destination) {
                std::cout << "MOVE " << destination->x << ' ' << destination->y << ';';
            }
            if (target != nullptr) {
                std::cout << "SHOOT " << target->id << '\n';
            } else {
                std::cout << "HUNKER_DOWN\n";
            }
        }
        // CodinGame waits for these commands before sending the next turn.
        std::cout.flush();
        ++turn_number;
    }
    return std::cin.eof() ? 0 : 1;
}
