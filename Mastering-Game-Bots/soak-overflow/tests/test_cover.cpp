// Include the submission unchanged, giving its entry point a different name.
#define main codingame_entry
#include "../main.cpp"
#undef main

#include <stdexcept>
#include <string>
#include <utility>

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void lookup_uses_tile_coordinates() {
    GameSetup map;
    map.width = 3;
    map.height = 2;
    map.tiles = {{2, 1, 0}, {1, 0, 2}, {0, 0, 0},
                 {2, 0, 1}, {0, 1, 0}, {1, 1, 0}};
    const auto* tile = tile_at(map, {1, 0});
    expect(tile != nullptr && tile->type == 2,
           "Lookup should find high cover by coordinate despite input order");
}

GameSetup map_with_cover(int type) {
    GameSetup map;
    map.width = 5;
    map.height = 5;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            map.tiles.push_back({x, y, x == 2 && y == 2 ? type : 0});
        }
    }
    return map;
}

void low_cover_protects_target_from_opposite_side() {
    const auto map = map_with_cover(1);
    expect(cover_reduction(map, {0, 2}, {3, 2}) == 50,
           "Low cover between shooter and adjacent target should reduce damage by 50%");
}

void high_cover_protects_target_from_opposite_side() {
    const auto map = map_with_cover(2);
    expect(cover_reduction(map, {0, 2}, {3, 2}) == 75,
           "High cover should reduce damage by 75%");
}

void strongest_cover_counts_without_stacking() {
    auto map = map_with_cover(0);
    for (auto& tile : map.tiles) {
        if (tile.x == 2 && tile.y == 3) {
            tile.type = 2;
        } else if (tile.x == 3 && tile.y == 2) {
            tile.type = 1;
        }
    }
    expect(cover_reduction(map, {0, 0}, {3, 3}) == 75,
           "When high and low covers both apply, only the 75% reduction counts");
}

void lookup_rejects_coordinates_outside_map() {
    const auto map = map_with_cover(2);
    for (const auto position : {Position{-1, 2}, Position{2, -1},
                                Position{5, 2}, Position{2, 5}}) {
        expect(tile_at(map, position) == nullptr,
               "A coordinate outside the map must not resolve to a tile");
    }
}

void empty_tiles_do_not_protect() {
    const auto map = map_with_cover(0);
    expect(cover_reduction(map, {0, 2}, {3, 2}) == 0,
           "An empty tile between agents provides no protection");
}

void cover_on_wrong_side_does_not_protect() {
    const auto map = map_with_cover(2);
    expect(cover_reduction(map, {4, 2}, {3, 2}) == 0,
           "Cover behind the target relative to the shooter does not protect");
}

void target_must_be_adjacent_to_cover() {
    const auto map = map_with_cover(2);
    expect(cover_reduction(map, {0, 2}, {4, 2}) == 0,
           "A target two tiles from cover receives no cover reduction");
}

void shooter_adjacent_to_same_cover_cancels_protection() {
    const auto map = map_with_cover(2);
    expect(cover_reduction(map, {1, 2}, {3, 2}) == 0,
           "Agents on opposite adjacent sides of the same cover are unprotected");
}

void diagonally_adjacent_shooter_cancels_protection() {
    const auto map = map_with_cover(2);
    expect(cover_reduction(map, {1, 1}, {3, 2}) == 0,
           "The referee also excludes cover beside a diagonally adjacent shooter");
}

void cover_works_for_other_incoming_directions() {
    const auto map = map_with_cover(1);
    expect(cover_reduction(map, {2, 0}, {2, 3}) == 50,
           "Low cover protects from a shot from above");
    expect(cover_reduction(map, {4, 2}, {1, 2}) == 50,
           "Low cover protects from a shot from the right");
    expect(cover_reduction(map, {2, 4}, {2, 1}) == 50,
           "Low cover protects from a shot from below");
}

void map_edge_target_without_cover_is_unprotected() {
    const auto map = map_with_cover(0);
    expect(cover_reduction(map, {4, 4}, {0, 0}) == 0,
           "Checking a target at the map edge must be safe");
}

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, void (*)()>> tests = {
        {"lookup", lookup_uses_tile_coordinates},
        {"low_cover", low_cover_protects_target_from_opposite_side},
        {"high_cover", high_cover_protects_target_from_opposite_side},
        {"strongest_cover", strongest_cover_counts_without_stacking},
        {"outside_map", lookup_rejects_coordinates_outside_map},
        {"empty", empty_tiles_do_not_protect},
        {"wrong_side", cover_on_wrong_side_does_not_protect},
        {"nonadjacent_target", target_must_be_adjacent_to_cover},
        {"adjacent_shooter", shooter_adjacent_to_same_cover_cancels_protection},
        {"diagonal_shooter", diagonally_adjacent_shooter_cancels_protection},
        {"other_directions", cover_works_for_other_incoming_directions},
        {"map_edge", map_edge_target_without_cover_is_unprotected},
    };
    bool found = false;
    try {
        for (const auto& [name, run] : tests) {
            if (argc == 1 || name == argv[1]) {
                found = true;
                run();
                std::cout << "PASS: " << name << '\n';
            }
        }
        expect(found, "No test matched the requested name");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
