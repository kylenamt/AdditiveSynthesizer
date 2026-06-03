# Wavetable vs std::sin Benchmark Report

## Running the Benchmark

```sh
cd miscs
g++ -O2 -std=c++17 -o benchmark_sine benchmark_sine.cpp && ./benchmark_sine
```

Swap `-O2` for `-O0` / `-O1` / `-O3` to compare optimisation levels.

## Setup

Simulates the real audio processing loop: N voices, each with P partials, each processed per sample.
Wavetable is 2048 points with linear interpolation (mirrors `SineWavetable.h`). All tests at 48 kHz, 512-sample block, 500 repetitions.

## Results — 8 voices × 512 partials (max load)

| Optimisation | Wavetable | std::sin | Speedup |
|---|---|---|---|
| `-O0` | 30676 ms | 23310 ms | 0.76× |
| `-O1` | 16440 ms | 5078 ms | 0.31× |
| `-O2` | 16614 ms | 5837 ms | 0.35× |
| `-O3` | 16003 ms | 5846 ms | 0.37× |

**std::sin is ~3× faster at every optimisation level above -O0.**

## Results — 4 voices × 256 partials (typical load)

| Optimisation | Wavetable | std::sin | Speedup |
|---|---|---|---|
| `-O0` | 8660 ms | 6910 ms | 0.80× |
| `-O1` | 463 ms | 1445 ms | 3.12× |
| `-O2` | 466 ms | 1998 ms | **4.29×** |
| `-O3` | 465 ms | 1994 ms | **4.29×** |

**Wavetable is ~4.3× faster at `-O2`/`-O3`.**


## Crossover Point

The wavetable wins at moderate load (≤ 4 voices × 256 partials) but loses at maximum load (8 voices × 512 partials). The likely cause is that at max load the compiler manages to auto-vectorise `std::sin` across the larger loop body, whereas at typical load the wavetable's arithmetic advantage dominates.
