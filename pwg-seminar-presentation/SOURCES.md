# Źródła i atrybucje

Wszystkie pliki z `assets/` są przechowywane lokalnie — prezentacja nie korzysta z linków zewnętrznych.

## 1. Materiały z projektu PWG (autorskie)

Wszystkie pliki w `assets/pwg/` pochodzą z własnych nagrań i materiałów autora projektu PWG. Nic nie zostało wygenerowane przez AI ani zastąpione materiałem zastępczym.

| Plik | Pochodzenie | Slajd |
|---|---|---|
| `lake-glint.jpg` | klatka z nagrania `Videos/mount1.mp4` (19.09.2026), kadr przycięty do widoku 3D | 00 |
| `lake-valley.jpg` | klatka z `mount1.mp4` | 05 |
| `params-a/b/c.jpg` | klatki z `Videos/2026-04-09 17-04-06.mp4` (wczesna wersja PWG, ta sama kamera) | 07 |
| `mask-elev/mountain/height/final.png` | podglądy TerrainDebug ze sprawozdania z postępów (`Desktop/Sprawozdanie_z_postepów_projektu/images/elev.png`, `mountain.png`, `height.png`, `final.png`); kanał R przekonwertowany na skalę szarości | 08, 11 |
| `erosion-sim.mp4`, `erosion-sim-poster.jpg` | nagranie `assets/pwg/nowe/2026-10-20 19-19-58 - Trim.mp4` (20.10.2026): symulacja erozji z podglądem cząstek, stała kamera; bez pierwszych 0,5 s (kursor), bez dźwięku, 1280 × 720 | 10 |
| `pwg-demo.mp4`, `pwg-demo-poster.jpg` | montaż ok. 51 s (bez efektów) z przelotów w trybie eksploracji: `mount1.mp4` (12–30 s), `mount2.mp4` (112–127 s), `nowe/pustynia.mp4` (5–15 s), `mount1.mp4` (50–58 s) | 14 |
| `desert-dunes.jpg` | klatka z `assets/pwg/nowe/pustynia.mp4` (9 s) | 17 (drugie tło) |
| `dusk-ridge.jpg` | klatka z `mount2.mp4` | 15 |
| `mountain-snow.jpg` | klatka z `mount2.mp4` | 16 |
| `tree-ridge.jpg` | klatka z `mount2.mp4` | 17 |

Pozostałe klatki w `assets/pwg/` (`island-*.jpg`, `lake-cliffs.jpg`, `river-grass.jpg`, `slope-river.jpg`, `graph-*.jpg`, `editor-ui.jpg`, `editor-materials.jpg`) są zapasowymi zrzutami do ewentualnej podmiany. Folder `assets/pwg/nowe/` zawiera surowe nagrania źródłowe i jest wyłączony z repozytorium (`.gitignore`).

Ilustracje na slajdach 06, 10, 12 i 13 (siatka, przekrój erozji, mapa chunków, siatki LOD) są **ilustracjami poglądowymi** rysowanymi w przeglądarce. Dane liczbowe na nich pochodzą z kodu PWG:
- 5 poziomów LOD, krok `1 << lod`, chunk 129 wierzchołków → 128/64/32/16/8 kwadratów na bok: `src/terrain/lod/LODManager.cpp`, `src/graphics/mesh/meshManager.cpp` (`CreatePlaneMesh`).
- Liczba chunków 3–31 na bok: `src/terrain/chunk/chunkManager.cpp` (`ConfigureTerrain`).
- Grupy robocze 8 × 8: `assets/shaders/terrainShaders/**/*.comp` (`local_size_x = 8, local_size_y = 8`).
- Finalna wysokość jako ważona suma masek podniesiona do potęgi 1,5: `assets/shaders/terrainShaders/masks/finalMask.comp`.
- Stan funkcji (erozja hydrauliczna i termiczna, hydrologia, woda, ograniczenia, brak testów automatycznych): `docs/PROJECT_DESCRIPTION.md` (stan na 21.09.2026), zweryfikowany wyrywkowo z kodem.

## 2. Kadry z gier i filmów (materiały osób trzecich)

Kadry pochodzą z oficjalnych materiałów wydawców i studiów. Użyto ich wyłącznie jako ilustracji w niekomercyjnym wystąpieniu dydaktycznym. Przed publikacją prezentacji w internecie sprawdź z promotorem, czy takie użycie mieści się w prawie cytatu (art. 29 ustawy o prawie autorskim) i w dozwolonym użytku edukacyjnym (art. 27). Każdy kadr ma na slajdzie podpis z właścicielem praw.

| Plik | Tytuł / prawa | Źródło (oryginalny URL) |
|---|---|---|
| `ref/minecraft.jpg` | Minecraft © Mojang Studios / Microsoft | https://www.minecraft.net/content/dam/minecraftnet/games/minecraft/screenshots/PDP-Gallery_OV_2_16x9.jpg (galeria na https://www.minecraft.net/en-us/store/minecraft-java-bedrock-edition-pc) |
| `ref/nms-meadow.jpg` | No Man's Sky © Hello Games | https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/275850/6c2ddcffe192b0e155a69aa2a1dd3f33b863033d/ss_6c2ddcffe192b0e155a69aa2a1dd3f33b863033d.1920x1080.jpg (sklep Steam, app 275850) |
| `ref/nms-rings.jpg` | No Man's Sky © Hello Games | https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/275850/1712db8fdd282cb1a4c87598148e0fbb9bc0b744/ss_1712db8fdd282cb1a4c87598148e0fbb9bc0b744.1920x1080.jpg |
| `ref/horizon.jpg` | Horizon Zero Dawn © Guerrilla / Sony Interactive Entertainment | https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/1151640/ss_9db45aa04e8c8b5043b479f42ed36296bfc3a918.1920x1080.jpg (Steam, app 1151640; przycięte czarne pasy) |
| `ref/good-dinosaur.jpg` | Dobry dinozaur (2015) © Disney / Pixar | https://lumiere-a.akamaihd.net/v1/images/h_thegooddinosaur_19753_1a1d9592.jpeg (https://movies.disney.com/the-good-dinosaur) |
| `ref/good-dinosaur-valley.jpg` | Dobry dinozaur (2015) © Disney / Pixar | https://lumiere-a.akamaihd.net/v1/images/image_fcc88884.jpeg (https://movies.disney.com/the-good-dinosaur) |
| `ref/lion-king-pride.jpg` | Król Lew (2019) © Disney | https://lumiere-a.akamaihd.net/v1/images/g_lionking2019_03_17788_753d9ac3.jpeg (https://movies.disney.com/the-lion-king-2019) |

Nieużyte w prezentacji, zostawione jako zapas: `ref/lion-king-lions.jpg` (https://lumiere-a.akamaihd.net/v1/images/image_03c86a38.jpeg) i `ref/horizon-b.jpg` (Steam, app 1151640).

## 3. Źródła stwierdzeń faktograficznych

### Przykłady z przemysłu (slajdy 01–04)

- **No Man's Sky — architektura generowania.** I. McKendrick, *Continuous World Generation in No Man's Sky*, GDC 2017. https://gdcvault.com/play/1024265/Continuous-World-Generation-in-No
- **No Man's Sky — matematyka światów.** S. Murray, *Building Worlds Using Math(s)*, GDC 2017. https://gdcvault.com/play/1024514/Building-Worlds-Using
- **No Man's Sky — 18 446 744 073 709 551 616 (2⁶⁴) możliwych planet wyznaczanych przez seed.** Wypowiedź S. Murraya z 2014 r. cytowana m.in. przez Gematsu: https://gematsu.com/2014/08/mans-sky-18-quintillion-planets. Patrz też https://en.wikipedia.org/wiki/No_Man%27s_Sky.
- **Horizon Zero Dawn — proceduralne rozmieszczanie obiektów na GPU** (nie generowanie terenu). J. van Muijden, *GPU-Based Procedural Placement in Horizon Zero Dawn*, GDC 2017.
  - slajdy: https://www.guerrilla-games.com/media/News/Files/GDC2017_VanMuijden_GPUBasedProceduralPlacementInHorizonZeroDawn.pdf
  - artykuł: https://www.guerrilla-games.com/read/gpu-based-procedural-placement-in-horizon-zero-dawn
  - GDC Vault: https://www.gdcvault.com/play/1024120/GPU-Based-Run-Time-Procedural
- **Minecraft — teren z funkcji szumu i seeda.**
  - https://minecraft.wiki/w/Tutorial:Custom_world_generation
  - https://minecraft.fandom.com/wiki/Noise_generator
- **Dobry dinozaur — krajobrazy z danych wysokościowych i satelitarnych USGS.**
  - https://www.slashfilm.com/making-of-the-good-dinosaur
  - https://planetizen.com/node/82561
  - https://www.eugenewei.com/blog/2015/10/13/the-good-dinosaur
- **Król Lew (2019) — proceduralne środowiska MPC (66 planów, ok. 150 km², bez klasycznego matte paintingu).**
  - befores & afters: https://beforesandafters.com/2019/08/13/how-the-lion-king-moved-away-from-digital-matte-paintings-to-create-fully-cg-vistas/ (cytuje referat SIGGRAPH 2019 *DMP without DMP, full-CG environments for The Lion King*, Bonatti, Rolandi, Bolbach, Wolter)
  - MPC: https://www.mpcvfx.com/en/news/advancing-technologies-for-the-lion-king/
- **16,7 ms na klatkę przy 60 FPS** — wartość obliczona (1000 ms / 60).

### Algorytmy i technologie (slajdy 06–13)

Spójne z `docs/LITERATURE.md` w repozytorium PWG:

- K. Perlin, *An Image Synthesizer*, ACM SIGGRAPH Computer Graphics 19(3), 1985. https://doi.org/10.1145/325165.325247
- F. K. Musgrave, C. E. Kolb, R. S. Mace, *The Synthesis and Rendering of Eroded Fractal Terrains*, SIGGRAPH '89. https://doi.org/10.1145/74333.74337
- X. Mei, P. Decaudin, B.-G. Hu, *Fast Hydraulic Erosion Simulation and Visualization on GPU*, Pacific Graphics 2007. https://doi.org/10.1109/PG.2007.15
- D. G. Tarboton, *A New Method for the Determination of Flow Directions and Upslope Areas in Grid Digital Elevation Models*, Water Resources Research 33(2), 1997. https://doi.org/10.1029/96WR03137
- R. Barnes, C. Lehman, D. Mulla, *Priority-Flood: An Optimal Depression-Filling and Watershed-Labeling Algorithm for Digital Elevation Models*, Computers & Geosciences 62, 2014. https://doi.org/10.1016/j.cageo.2013.04.024
- F. Losasso, H. Hoppe, *Geometry Clipmaps: Terrain Rendering Using Nested Regular Grids*, ACM TOG 23(3), 2004. https://doi.org/10.1145/1015706.1015799
- FastNoiseLite: https://github.com/Auburn/FastNoiseLite
- OpenGL 4.5 Core Profile Specification (compute shaders): https://registry.khronos.org/OpenGL/specs/gl/glspec45.core.pdf

### Projekt prezentacji

- D. JP Phillips, *How to Avoid Death by PowerPoint*, TEDxStockholmSalon. https://www.youtube.com/watch?v=Iwpi1Lm6dFo
- Architektura HTML: skill `frontend-slides` (zarazhangrui). https://github.com/zarazhangrui/frontend-slides

## 4. Fonty

Bricolage Grotesque i JetBrains Mono (licencja SIL Open Font License 1.1) zostały pobrane z Google Fonts i zapisane lokalnie w `assets/fonts/`, w podzbiorach latin i latin-ext (polskie znaki).
