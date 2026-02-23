# Tímový Projekt 2026 — ParPSys (T5: Intel oneAPI TBB)

## Prehľad projektu

Cieľom projektu je implementovať **3 výpočtové úlohy** (1 povinná + 2 voliteľné), pričom každú riešime v **sekvenčnej** (C/C++) a **paralelnej** (Intel oneAPI TBB) verzii. Porovnáme výkon oboch prístupov a zmeriame zrýchlenie (speedup).

### Zvolená technológia: Intel oneAPI Threading Building Blocks (TBB)

**Čo je TBB?**
- C++ knižnica pre paralelné programovanie na zdieľanej pamäti od Intelu (súčasť oneAPI).
- Abstrahuje nízkoúrovňové vlákna — programátor definuje **úlohy (tasky)**, TBB ich automaticky rozdeľuje medzi vlákna.
- Podporuje task-based paralelizmus, paralelné cykly, pipeline, flow graph a ďalšie vzory.
- Funguje na Linux/WSL, kompatibilné s GCC, Clang a Intel kompilátormi.

**Kľúčové koncepty TBB:**
| Koncept | Popis |
|---|---|
| `tbb::parallel_for` | Paralelné rozdelenie iterácií cyklu |
| `tbb::parallel_reduce` | Paralelná redukcia (agregácia) |
| `tbb::parallel_pipeline` | Pipeline paralelizmus (producer/consumer) |
| `tbb::flow::graph` | Dataflow grafy pre komplexné závislosti |
| `tbb::mutex` / `tbb::spin_mutex` | Synchronizačné primitíva |
| `tbb::concurrent_*` | Thread-safe kontajnery (hash_map, queue, vector) |
| `tbb::task_arena` | Kontrola počtu vlákien |

**Prostredie:** Linux / WSL2 na Windows, kompilácia cez `g++` / `icpx` s `-ltbb`.

---

## Úloha 1 — Maticové násobenie (Povinná)

### O čom to je
Klasické násobenie dvoch matíc $C = A \times B$ s komplexnosťou $O(n^3)$. Ideálny kandidát na paralelizáciu, pretože výpočet každého prvku $C[i][j]$ je nezávislý.

### Sekvenčná verzia (C/C++)
- Trojitý vnorený cyklus, prípadne optimalizácia poradím cyklov (ikj) pre cache-friendliness.
- Dátové typy: `int` a `float`.

### Paralelná verzia (TBB)
- `tbb::parallel_for` na vonkajší cyklus (riadky matice C).
- Alternatívne: 2D `tbb::blocked_range2d` pre blokovú dekompozíciu.
- Kontrola vlákien cez `tbb::global_control` alebo `tbb::task_arena`.

### Parametre meraní
| Parameter | Hodnoty |
|---|---|
| Rozmery matíc | 500×500, 1000×1000, 2000×2000 |
| Počet vlákien | 2, 4, 8 |
| Dátové typy | `int`, `float` |

### Čo budeme merať
- Čas sekvenčného vs. paralelného behu
- Speedup = $T_{seq} / T_{par}$
- Efektívnosť = $\text{Speedup} / p$ (kde $p$ = počet vlákien)

---

## Úloha 2 — Spracovanie obrázkov: Gaussovský filter (blur) (Voliteľná)

### O čom to je
Aplikácia konvolučného filtra (Gaussian blur) na obrázok — každý pixel sa prepočíta ako vážený priemer jeho okolia. Táto operácia je **embarrassingly parallel** — výpočet každého výstupného pixelu je nezávislý od ostatných.

### Prečo je vhodná
- **Jasný sekvenčný vs. paralelný kontrast:** Sekvenčne prechádzame pixel po pixeli, paralelne môžeme spracovať celé bloky/riadky naraz.
- **Výpočtovo náročná:** Pre veľké obrázky (4K+) a veľké jadrá (napr. 15×15) je sekvenčný výpočet pomalý.
- **Ideálna pre TBB:** `tbb::parallel_for` s `blocked_range2d` prirodzene mapuje na 2D pole pixelov.
- **Žiadna synchronizácia** (read-only vstup, nezávislý výstup) — čistý dátový paralelizmus.

### Implementačný plán
- **Sekvenčná verzia:** Dvojitý cyklus cez pixely, pre každý pixel vnorený cyklus cez konvolučné jadro.
- **Paralelná verzia:** `tbb::parallel_for` + `tbb::blocked_range2d<int>` na rozdelenie obrázka na bloky.
- **Formáty:** PPM/PGM (jednoduché na načítanie) alebo s knižnicou stb_image.

### Parametre meraní
| Parameter | Hodnoty |
|---|---|
| Rozlíšenie obrázka | 1920×1080, 3840×2160, 7680×4320 |
| Veľkosť jadra | 5×5, 11×11, 21×21 |
| Počet vlákien | 2, 4, 8 |

---

## Úloha 3 — Producer-Consumer pipeline: Spracovanie logov / dát (Voliteľná, so synchronizáciou)

### O čom to je
Simulácia pipeline spracovania veľkého množstva dátových záznamov (napr. log súborov, senzorových dát), kde:
1. **Producer** číta/generuje záznamy
2. **Spracovateľ(e)** parsujú a transformujú záznamy (napr. filtrovanie, agregácia, štatistiky)
3. **Consumer** zapisuje výsledky

Toto je **pipeline paralelizmus** — jednotlivé fázy bežia súčasne na rôznych dátach.

### Prečo je vhodná
- **Vyžaduje synchronizáciu:** Producer a consumeri zdieľajú frontu → nutnosť použitia `tbb::concurrent_bounded_queue` alebo manuálneho `tbb::mutex` / semaforu / bariéry → **spĺňa podmienku zadania**.
- **Odlišný vzor od Úlohy 1 a 2:** Nie je to dátový paralelizmus, ale pipeline/task paralelizmus.
- **Jasný sekvenčný kontrast:** Sekvenčne: čítaj → spracuj → zapíš jeden po druhom. Paralelne: všetky fázy bežia naraz.
- **Výborne podporené v TBB:** `tbb::parallel_pipeline` s filtrami, alebo `tbb::flow::graph`.

### Implementačný plán
- **Sekvenčná verzia:** Jednoduchý cyklus: read → process → write.
- **Paralelná verzia (Varianta A):** `tbb::parallel_pipeline` s 3 filtrami:
  - `serial_in_order` filter (čítanie)
  - `parallel` filter (spracovanie)
  - `serial_in_order` filter (zápis)
- **Paralelná verzia (Varianta B):** `tbb::flow::graph` s explicitnými uzlami + `tbb::concurrent_bounded_queue` + `tbb::mutex` na ochranu zdieľaných štatistík.
- **Synchronizácia:** `tbb::mutex` na ochranu výstupného súboru a zdieľaných akumulátorov, `tbb::concurrent_bounded_queue` na bounded buffer medzi fázami.

### Parametre meraní
| Parameter | Hodnoty |
|---|---|
| Počet záznamov | 100 000, 1 000 000, 10 000 000 |
| Počet vlákien | 2, 4, 8 |
| Veľkosť buffera | 1000, 10 000 |

---

## Alternatívne návrhy pre voliteľné úlohy

Ak by sa vyššie uvedené návrhy nehodili, tu sú ďalšie možnosti:

### Alt A: AES šifrovanie/dešifrovanie (kryptografia)
- Šifrovanie veľkého súboru po blokoch (ECB/CTR mód — bloky nezávislé).
- `tbb::parallel_for` na bloky, žiadna synchronizácia (alebo CTR s atomickým počítadlom).
- Dobrý kontrast: sekvenčne blok po bloku vs. paralelne všetky naraz.

### Alt B: Trénovanie jednoduchého MLP (umelá inteligencia)
- Forward pass + backpropagation na mini-batchoch.
- Paralelizácia cez `tbb::parallel_for` na matice váh alebo vzorky v batchoch.
- Synchronizácia pri aktualizácii váh (gradient accumulation + mutex/bariéra).

### Alt C: Paralelné triedenie (merge sort / bitonic sort)
- Rekurzívne delenie poľa → paralelné triedenie podpolí → merge.
- `tbb::parallel_invoke` na rekurzívne vetvy.
- Jednoduché, ale jasne ukazuje speedup.

---

## Štruktúra projektu

```
Paralel/
├── overview.md                    ← tento súbor
├── gift.txt                       ← 4 GIFT otázky k voliteľným úlohám
├── merania/
│   └── report.xlsx                ← výsledky meraní a grafy
├── prezentacia/
│   └── prezentacia.pptx           ← prezentácia s poznámkami
└── ulohy/
    ├── 1-povinna/
    │   ├── CMakeLists.txt
    │   ├── src/
    │   │   ├── main_seq.cpp       ← sekvenčná verzia (C++)
    │   │   └── main_tbb.cpp       ← paralelná verzia (TBB)
    │   └── include/
    │       └── matrix.h           ← spoločné funkcie
    ├── 2-volitelna/
    │   ├── CMakeLists.txt
    │   ├── src/
    │   │   ├── main_seq.cpp
    │   │   └── main_tbb.cpp
    │   └── include/
    │       └── ...
    └── 3-volitelna/
        ├── CMakeLists.txt
        ├── src/
        │   ├── main_seq.cpp
        │   └── main_tbb.cpp
        └── include/
            └── ...
```

---

## Profilácia

Použijeme **Intel VTune Profiler** (súčasť oneAPI) alebo **perf** (Linux) na identifikáciu hotspotov v sekvenčnom kóde pred paralelizáciou.

Postup:
1. Skompilovať sekvenčnú verziu s `-g` (debug info).
2. Spustiť profiláciu (`vtune -collect hotspots ./program`).
3. Identifikovať najpomalšie funkcie/cykly.
4. Tieto časti paralelizovať pomocou TBB.
5. Zahrnúť screenshoty z profilátora do prezentácie.

---

## Prostredie a kompilácia

### Inštalácia TBB na WSL2 (Ubuntu)
```bash
# Cez apt (odporúčané)
sudo apt update
sudo apt install libtbb-dev

# Alebo cez Intel oneAPI
wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
echo "deb https://apt.repos.intel.com/oneapi all main" | sudo tee /etc/apt/sources.list.d/oneAPI.list
sudo apt update
sudo apt install intel-oneapi-tbb-devel
```

### Kompilácia
```bash
# Sekvenčná verzia
g++ -O2 -o main_seq src/main_seq.cpp

# Paralelná verzia s TBB
g++ -O2 -o main_tbb src/main_tbb.cpp -ltbb

# S CMake
mkdir build && cd build
cmake ..
make
```

*Technológia: Intel oneAPI TBB | Prostredie: Linux/WSL2 | Jazyk: C++*
