#include <algorithm>
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
