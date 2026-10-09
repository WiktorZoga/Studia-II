#include <algorithm>
#include <cstdlib>
#include <iostream>
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

const AgentState* wettest_enemy(const GameSetup& setup, const TurnState& turn) {
    const AgentState* target = nullptr;
    // Preserve initial enemy order when wetness is tied, as in the referee.
    for (const auto& definition : setup.agents) {
        if (definition.player_id == setup.my_id) {
            continue;
        }
        const auto enemy = std::ranges::find(
            turn.agents, definition.id, &AgentState::id);
        if (enemy != turn.agents.end()
            && (target == nullptr || enemy->wetness > target->wetness)) {
            target = &*enemy;
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
        const auto* target = wettest_enemy(setup, turn);
        for (const auto* agent : owned_agents) {
            std::cout << agent->id << ';';
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
