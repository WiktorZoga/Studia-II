# Local checks

From the course root:

```sh
cmake -S soak-overflow -B soak-overflow/build -DCMAKE_BUILD_TYPE=Debug
cmake --build soak-overflow/build
ctest --test-dir soak-overflow/build --output-on-failure
```

The active checks are [cover geometry](test_cover.cpp) and
[Bronze input/output examples](test_bronze.py). The latter keep stdin open while
waiting for commands, check legal movement/combat and friendly safety, and cover
territory, wetness thresholds, range, cooldown, splash, and changing survivors.
Small boards isolate worked rule examples; generated states exercise the actual
12–20 by 6–10 board limits and 6–10 initial agents, for both player IDs.

[test_protocol.py](test_protocol.py) retains the earlier shooting, cover, and bomb
tutorial fixtures and expectations, and supplies the shared process driver.
Its tutorial policy tests apply to previous bot versions, not the active Bronze
bot. For the last bomb-tutorial version, use local commit `5eb2388`.
No stage is inferred from bomb inventory.

The timing example writes `build/bronze-timing.json`, measuring Python
send/read/validation wall time, including harness overhead. These are local
observations, not a CodinGame performance guarantee. Scripted turn inputs and
generated states are not a full referee simulation and do not establish wins.
