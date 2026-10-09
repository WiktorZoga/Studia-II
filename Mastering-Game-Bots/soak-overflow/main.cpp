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

bool same_position(Position a, Position b) {
    return a.x == b.x && a.y == b.y;
}

const AgentDefinition& definition_for(const GameSetup& setup, int id) {
    return *std::ranges::find(setup.agents, id, &AgentDefinition::id);
}

struct AgentAction {
    Position destination;
    const AgentState* target = nullptr;
    std::optional<Position> aim;
};

std::vector<Position> legal_positions(
    const GameSetup& setup, const TurnState& turn, const AgentState& agent,
    const std::vector<AgentAction>& planned) {
    std::vector<Position> positions{{agent.x, agent.y}};
    for (const auto next : adjacent(positions.front())) {
        const auto* tile = tile_at(setup, next);
        if (tile == nullptr || tile->type != 0) {
            continue;
        }
        const bool occupied = std::ranges::any_of(turn.agents, [next](const auto& other) {
            return same_position(next, {other.x, other.y});
        });
        const bool reserved = std::ranges::any_of(planned, [next](const auto& action) {
            return same_position(next, action.destination);
        });
        if (!occupied && !reserved) {
            positions.push_back(next);
        }
    }
    return positions;
}

// Path distances guide an approach around walls; they are not territory distances.
std::vector<int> approach_distances(const GameSetup& setup, const TurnState& turn) {
    const int cells = setup.width * setup.height;
    const auto index = [&](Position p) { return p.y * setup.width + p.x; };
    std::vector<int> distance(cells, cells + 1);
    std::vector<Position> queue;
    for (const auto* enemy : surviving_enemies(setup, turn)) {
        const Position position{enemy->x, enemy->y};
        distance[index(position)] = 0;
        queue.push_back(position);
    }
    for (std::size_t head = 0; head < queue.size(); ++head) {
        for (const auto next : adjacent(queue[head])) {
            const auto* tile = tile_at(setup, next);
            if (tile == nullptr || tile->type != 0) {
                continue;
            }
            // Influence paths may pass through agents; actual steps cannot.
            if (distance[index(next)] > distance[index(queue[head])] + 1) {
                distance[index(next)] = distance[index(queue[head])] + 1;
                queue.push_back(next);
            }
        }
    }
    return distance;
}

int shot_damage(
    const GameSetup& setup, const AgentDefinition& shooter,
    Position from, Position target, bool hunkered = false) {
    const int distance = manhattan(from, target);
    if (distance > 2 * shooter.optimal_range) {
        return 0;
    }
    const int range_factor = distance <= shooter.optimal_range ? 2 : 1;
    const int protection = cover_reduction(setup, from, target) + (hunkered ? 25 : 0);
    // Integer form of the referee's positive Math.round(power * range * protection).
    return (shooter.soaking_power * range_factor * (100 - protection) + 100) / 200;
}

int attack_benefit(int damage, int wetness) {
    const int remaining = std::max(0, 100 - wetness);
    if (remaining == 0 || damage == 0) {
        return 0;
    }
    return std::min(damage, remaining) + (damage >= remaining ? 20 : 0);
}

struct ShotChoice {
    const AgentState* target = nullptr;
    int benefit = 0;
};

ShotChoice best_shot(
    const GameSetup& setup, const TurnState& turn,
    const AgentState& agent, Position position) {
    ShotChoice best;
    if (agent.cooldown != 0) {
        return best;
    }
    const auto& definition = definition_for(setup, agent.id);
    for (const auto* enemy : surviving_enemies(setup, turn)) {
        const int damage = shot_damage(setup, definition, position, {enemy->x, enemy->y});
        const int benefit = attack_benefit(damage, enemy->wetness);
        if (benefit > best.benefit) {
            best = {enemy, benefit};
        }
    }
    return best;
}

struct Exposure {
    int damage = 0;
    int hunkered_damage = 0;

    int defense_benefit() const { return damage - hunkered_damage; }
};

Exposure shooting_exposure(const GameSetup& setup, const TurnState& turn, Position position) {
    Exposure exposure;
    // Enemy moves/hunkering are not observable. Use their current positions;
    // include every ready shooter even if our planned attack could be lethal.
    for (const auto* enemy : surviving_enemies(setup, turn)) {
        if (enemy->cooldown != 0) {
            continue;
        }
        const auto& definition = definition_for(setup, enemy->id);
        exposure.damage += shot_damage(setup, definition, {enemy->x, enemy->y}, position);
        exposure.hunkered_damage += shot_damage(setup, definition, {enemy->x, enemy->y}, position, true);
    }
    return exposure;
}

int territory_advantage(
    const GameSetup& setup, const TurnState& projected, int moving_id, Position candidate) {
    int advantage = 0;
    for (const auto& tile : setup.tiles) {
        int friendly = 10000, enemy = 10000;
        for (const auto& agent : projected.agents) {
            const Position position = agent.id == moving_id ? candidate : Position{agent.x, agent.y};
            const int distance = manhattan(position, {tile.x, tile.y})
                               * (agent.wetness >= 50 ? 2 : 1);
            if (definition_for(setup, agent.id).player_id == setup.my_id) {
                friendly = std::min(friendly, distance);
            } else {
                enemy = std::min(enemy, distance);
            }
        }
        advantage += (friendly < enemy) - (enemy < friendly);
    }
    return advantage;
}

struct SplashChoice {
    std::optional<Position> aim;
    int benefit = 0;
};

SplashChoice best_splash(
    const GameSetup& setup, const TurnState& anticipated, const AgentState& agent,
    Position from, const std::vector<const AgentState*>& owned,
    const std::vector<AgentAction>& actions) {
    SplashChoice best;
    if (agent.splash_bombs <= 0) {
        return best;
    }
    for (const auto& tile : setup.tiles) {
        const Position aim{tile.x, tile.y};
        if (manhattan(from, aim) > 4) {
            continue;
        }
        bool friendly_hit = false;
        for (std::size_t i = 0; i < owned.size(); ++i) {
            // Protect both the planned destination and the old cell in case
            // simultaneous opponent movement causes a move to be cancelled.
            if (in_blast(aim, actions[i].destination)
                || in_blast(aim, {owned[i]->x, owned[i]->y})) {
                friendly_hit = true;
                break;
            }
        }
        if (friendly_hit) {
            continue;
        }
        int benefit = 0;
        for (const auto* enemy : surviving_enemies(setup, anticipated)) {
            if (in_blast(aim, {enemy->x, enemy->y})) {
                benefit += attack_benefit(30, enemy->wetness);
            }
        }
        if (benefit > best.benefit) {
            best = {aim, benefit};
        }
    }
    return best;
}

std::vector<AgentAction> plan_actions(
    const GameSetup& setup, const TurnState& turn,
    const std::vector<const AgentState*>& owned) {
    std::vector<AgentAction> actions;
    TurnState projected = turn;
    const auto distances = approach_distances(setup, turn);
    const auto distance = [&](Position p) { return distances[p.y * setup.width + p.x]; };
    for (const auto* agent : owned) {
        Position destination{agent->x, agent->y};
        const auto value_at = [&](Position candidate) {
            const auto shot = best_shot(setup, turn, *agent, candidate);
            const auto exposure = shooting_exposure(setup, turn, candidate);
            // A deliberately simple baseline: balance tile advantage against
            // twice the useful damage dealt or avoided, minus incoming damage.
            return territory_advantage(setup, projected, agent->id, candidate)
                 + 2 * (std::max(shot.benefit, exposure.defense_benefit()) - exposure.damage);
        };
        int best_value = value_at(destination);
        for (const auto candidate : legal_positions(setup, turn, *agent, actions)) {
            const int value = value_at(candidate);
            if (value > best_value || (value == best_value && distance(candidate) < distance(destination))) {
                destination = candidate;
                best_value = value;
            }
        }
        auto& moved = *std::ranges::find(projected.agents, agent->id, &AgentState::id);
        moved.x = destination.x;
        moved.y = destination.y;
        actions.push_back({destination, nullptr, std::nullopt});
    }

    // Fix all team destinations before choosing blasts. Nothing moves into a
    // previously selected blast later in this plan; these plans reset each turn.
    TurnState anticipated = turn;
    for (std::size_t i = 0; i < owned.size(); ++i) {
        const auto& agent = *owned[i];
        auto& action = actions[i];
        const auto shot = best_shot(setup, anticipated, agent, action.destination);
        const auto exposure = shooting_exposure(setup, turn, action.destination);
        const auto splash = best_splash(setup, anticipated, agent, action.destination, owned, actions);
        if (splash.benefit > std::max(shot.benefit, exposure.defense_benefit())) {
            action.aim = splash.aim;
            for (auto& enemy : anticipated.agents) {
                if (definition_for(setup, enemy.id).player_id != setup.my_id
                    && in_blast(*action.aim, {enemy.x, enemy.y})) {
                    enemy.wetness += 30;
                }
            }
        } else if (shot.benefit > exposure.defense_benefit()) {
            // Keep the command pointer in the input turn, not the temporary
            // damage ledger. Future opponent movement/hunkering is uncertain.
            action.target = &*std::ranges::find(turn.agents, shot.target->id, &AgentState::id);
            auto& target = *std::ranges::find(anticipated.agents, shot.target->id, &AgentState::id);
            target.wetness += shot_damage(setup, definition_for(setup, agent.id),
                                         action.destination, {target.x, target.y});
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
        std::ranges::sort(owned_agents, {}, &AgentState::id);
        const auto actions = plan_actions(setup, turn, owned_agents);
        for (std::size_t i = 0; i < owned_agents.size(); ++i) {
            const auto* agent = owned_agents[i];
            const auto& action = actions[i];
            std::cout << agent->id << ';';
            if (!same_position(action.destination, {agent->x, agent->y})) {
                std::cout << "MOVE " << action.destination.x << ' ' << action.destination.y << ';';
            }
            if (action.aim) {
                std::cout << "THROW " << action.aim->x << ' ' << action.aim->y << '\n';
            } else if (action.target != nullptr) {
                std::cout << "SHOOT " << action.target->id << '\n';
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
