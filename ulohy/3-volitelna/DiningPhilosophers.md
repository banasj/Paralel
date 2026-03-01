# Problém hodujúcich filozofov (Dining Philosophers)

## Čo je to?

Klasický problém synchronizácie: N filozofov sedí okolo okrúhleho stola. Medzi každými dvoma susednými filozofmi leží jedna vidlička. Filozof striedavo **myslí** a **je**, pričom na jedenie potrebuje **obe susedné vidličky**.

```
        F0
      🍴    🍴
    F4        F1
   🍴          🍴
    F3        F2
      🍴    🍴

F0..F4 = filozofi
🍴 = vidličky (zdieľané medzi susedmi)
```

---

## Sekvenčná verzia

```
for každé jedlo:
    for každý filozof:
        mysli()          ← výpočtová záťaž
        zdvihni vidličky ← žiadny konflikt (sekvenčne)
        jedz()           ← výpočtová záťaž
        polož vidličky
```

Žiadna synchronizácia — filozofi jedia jeden po druhom.

---

## Paralelná verzia — kde je paralelizmus?

Filozofi **myslia** nezávisle → môžu myslieť paralelne. Jedenie vyžaduje exkluzívny prístup k vidličkám.

```
Vlákno 0 (F0): myslí...    | čaká na vidličku 0,1 | je     | myslí...
Vlákno 1 (F1): myslí...    | čaká...              | čaká...| je
Vlákno 2 (F2): myslí...    | je (vidličky 2,3)    | myslí...
```

---

## Prepínanie synchronizácie (SyncMode flag)

Program podporuje dva režimy, prepínateľné pomocou `SyncMode` enum a CLI flagov:

| Režim | CLI flag | Popis |
|-------|----------|-------|
| `SYNC_ORDERED` | `--sync` | Mutexy + resource ordering — korektné riešenie |
| `SYNC_NONE` | `--no-sync` | Žiadne mutexy — úmyselné data race na demonštráciu |

Bez flagu program spustí **obe verzie** a zobrazí porovnávaciu tabuľku.

### Čo demonštruje SYNC_NONE?

1. **Lost updates na zdieľanom countri** — `total_meals_shared` je `int` (nie `atomic`), takže concurrent `read-modify-write` operácie vedú k strate inkrementov
2. **Fork violations** — dvaja susední filozofi simultánne "držia" tú istú vidličku, čo by v reálnom systéme znamenalo data corruption

### Prečo je total_meals_shared `int` a nie `atomic`?

Úmyselne! Ak by bol `atomic<int>`, inkrementácia by bola vždy korektná a nedokázali by sme demonštrovať problém. Práve `int` bez ochrany mutexom spôsobuje **lost updates** — viacero vlákien prečíta rovnakú hodnotu, inkrementuje ju a zapíše, čím sa stratia niektoré inkrementácie.

### Očakávané výsledky

| Metrika | SYNC_ORDERED | SYNC_NONE |
|---------|-------------|-----------|
| Per-filozof jedlá | Presne `num_meals` | Presne `num_meals` (per-vlákno lokálne) |
| `total_meals_shared` | `N * num_meals` | `< N * num_meals` (lost updates) |
| Fork violations | 0 | > 0 (simultánny prístup) |
| Deadlock | Nie (resource ordering) | Nie (žiadne zamykanie) |
| Celková správnosť | PASS | FAIL |

> **Poznámka:** Per-filozof `meals_eaten` bude správny aj bez synchronizácie, pretože každý filozof má vlastnú štruktúru `PhilosopherStats` — žiadna kontencia. Chyby sa prejavia iba na **zdieľaných** zdrojoch (`total_meals_shared`, vidličky).

---

## Kde vzniká problém?

### Race condition na vidličkách

```
Vlákno 0 (F0): chce vidličku 0 → zamkni ✓
Vlákno 1 (F1): chce vidličku 0 → zamkni ✓    ← BEZ MUTEXU: data race!

Bez synchronizácie: obaja "držia" vidličku 0 → undefined behavior
```

### Deadlock (bez usporiadania zdrojov)

```
F0: zamkni ľavú (vidlička 0) ✓    → čaká na pravú (vidlička 1) ⏳
F1: zamkni ľavú (vidlička 1) ✓    → čaká na pravú (vidlička 2) ⏳
F2: zamkni ľavú (vidlička 2) ✓    → čaká na pravú (vidlička 3) ⏳
F3: zamkni ľavú (vidlička 3) ✓    → čaká na pravú (vidlička 4) ⏳
F4: zamkni ľavú (vidlička 4) ✓    → čaká na pravú (vidlička 0) ⏳  ← DEADLOCK!

Nikto neuvoľní svoju vidličku → program zamrzne navždy!
```

### Lost updates na zdieľanom countri

```
Vlákno 0: read total=10 → compute 10+1=11 → write total=11
Vlákno 1: read total=10 → compute 10+1=11 → write total=11   ← rovnaká hodnota!
                                                                  Stratený 1 inkrement!
Výsledok: total=11 namiesto 12
```

---

## Riešenie: Usporiadanie zdrojov (Resource Ordering)

```cpp
// Každý filozof zamyká vidličku s NIŽŠÍM indexom ako prvú
int first  = std::min(left, right);
int second = std::max(left, right);

tbb::mutex::scoped_lock lock1(forks[first]);
tbb::mutex::scoped_lock lock2(forks[second]);
```

Prečo to funguje:
- F0: zamkne 0, potom 1
- F1: zamkne 1, potom 2
- F4: zamkne **0**, potom 4 (nie 4 potom 0!)
- → Cyklická závislosť nie je možná → žiadny deadlock

---

## Typy synchronizácie

| Varianta | Technika | Deadlock-free? | Férovosť |
|----------|----------|----------------|----------|
| Žiadna sync | — | N/A (crash) | — |
| Každý zamkne ľavú prvú | `tbb::mutex` | ✗ Deadlock! | — |
| Resource ordering | `tbb::mutex` + `min/max` | ✓ | Stredná |
| Asymetrický filozof | Posledný zamkne opačne | ✓ | Stredná |
| Waiter (semafor) | Max N-1 filozofov súčasne | ✓ | Dobrá |

---

## TBB konštrukty

| Konštrukt | Účel |
|-----------|------|
| `tbb::parallel_for` | Paralelné spustenie N filozofov |
| `tbb::mutex` | Ochrana vidličiek (zdieľaných zdrojov) |
| `tbb::mutex::scoped_lock` | RAII zamykanie — automatické uvoľnenie |
| `tbb::global_control` | Kontrola počtu vlákien |
| `std::atomic<int>` | Thread-safe sledovanie fork violations |

---

## Spustenie

```bash
# Obe verzie (SYNC + NO_SYNC) s porovnávanou tabuľkou
./main_tbb

# Iba synchronizovaná verzia
./main_tbb --sync

# Iba nesynchronizovaná verzia (demonštrácia chýb)
./main_tbb --no-sync

# Custom parametre: 10 filozofov, 100 jedál, intenzita 300, 4 vlákna
./main_tbb 10 100 300 4

# Kombinácia
./main_tbb 10 100 300 --no-sync
```

---

## Čo sa meria

| Parameter | Hodnoty |
|-----------|---------|
| Počet filozofov | 5, 10, 20 |
| Počet jedál | 10, 50, 100 |
| Intenzita práce | 200, 500, 1000 |
| Vlákna | 2, 4, 8 |

---

## Vzorový výstup

```
╔══════════════════════════════════════════════════════════════╗
║  Problém hodujúcich filozofov — Porovnanie synchronizácie   ║
╠══════════════════════════════════════════════════════════════╣
║  Filozofi:    5   Jedlá:   50   Intenzita:   500       ║
╚══════════════════════════════════════════════════════════════╝

========================================
  SEKVENČNÁ VERZIA (baseline)
========================================
Celkový čas: 245.67 ms

╔══════════════════════════════════════════════════════════════╗
║  TBB: 5 filozofov, 50 jedál, 4 vlákien
╚══════════════════════════════════════════════════════════════╝

--- Režim: SYNC_ORDERED (mutex + resource ordering) ---
--- Verifikácia ---
Per-filozof jedlá:   OK (súčet: 250 / 250)
Zdieľaný counter:    OK (250 / 250)
Fork violations:     OK (0)
Celková správnosť:   PASS ✓
TBB čas: 98.52 ms
Speedup vs sekvenčná: 2.49x

--- Režim: SYNC_NONE (bez mutexov — data race!) ---
--- Verifikácia ---
Per-filozof jedlá:   OK (súčet: 250 / 250)
Zdieľaný counter:    CHYBA! (237 / 250, stratených: 13 = 5.2%)
Fork violations:     CHYBA! (42 violations)
Celková správnosť:   FAIL ✗
TBB čas: 62.31 ms
Speedup vs sekvenčná: 3.94x

┌──────────────────┬──────────────────┬──────────────────┐
│                  │  SYNC_ORDERED    │  SYNC_NONE       │
├──────────────────┼──────────────────┼──────────────────┤
│ Čas [ms]         │            98.52 │            62.31 │
│ Speedup vs seq   │            2.49x │            3.94x │
│ Správnosť        │               OK │            CHYBA │
│ Fork violations  │                0 │               42 │
│ Shared counter   │              250 │              237 │
│ Očakávaný total  │              250 │              250 │
└──────────────────┴──────────────────┴──────────────────┘
```

---

## Prečo je tento príklad dobrý na prezentáciu

- **Klasický synchronizačný problém** — každý pozná dining philosophers
- **Jasný deadlock scenár** — bez usporiadania zdrojov program zamrzne
- **Mutex na zdieľaných zdrojoch** — vidličky chránené `tbb::mutex`
- **Prepínateľná synchronizácia** — jasný A/B dôkaz, že mutexy sú nevyhnutné
- **Dva typy chýb bez sync** — lost updates (shared counter) + fork violations
- **Merateľná kontencia** — čas čakania na vidličky ukazuje overhead synchronizácie
- **Férovosť** — porovnanie, či všetci filozofi jedia rovnako často
- **Škálovateľnosť** — viac filozofov a vlákien → väčší paralelizmus
- **Jednoduchá verifikácia** — každý filozof musí zjesť presne N jedál
