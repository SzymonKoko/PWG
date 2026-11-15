# Materiały PWG do ewentualnej podmiany

Prezentacja jest kompletna: każdy slajd ma prawdziwy materiał z PWG, wycięty z nagrań w `Videos/` i ze sprawozdania z postępów. Część z nich pochodzi jednak ze starszych wersji silnika albo nie pokazuje dokładnie tego, o czym mówi slajd. Poniższa lista mówi, co warto nagrać na aktualnej wersji, żeby prezentacja była najmocniejsza.

**Jak podmienić:** zapisz nowy plik pod **tą samą nazwą** w `assets/pwg/` (JPG 1920 × 1080; dla slajdu 07 format 4:3). Nie trzeba zmieniać HTML.

Jeśli plik zniknie albo będzie miał złą nazwę, slajd pokaże przerywaną ramkę „BRAK PLIKU” z oczekiwaną ścieżką. Nic nie zostanie podmienione po cichu.

## Priorytet wysoki

| Plik | Slajd | Co jest teraz | Co nagrać |
|---|---|---|---|
| `pwg-demo.mp4` | 14 | Montaż ok. 72 s: timelapse ×10 budowania terenu w edytorze grafu (21 s), potem przeloty w trybie eksploracji: góry z jeziorem → zbocza i rzeki → pustynia → widok całej mapy. | Opcjonalnie: timelapse z aktualnej wersji z podglądem masek debug. Plakat: `pwg-demo-poster.jpg`. |
| `params-a.jpg`, `params-b.jpg`, `params-c.jpg` | 07 | Wczesna wersja PWG (kwiecień 2026), płaski kolor, ta sama kamera. Na slajdzie podpisane jako „wczesna wersja”. | Trzy zrzuty z aktualnej wersji z **identycznej kamery**, różniące się jednym parametrem (np. frequency albo seed). Jeśli podmienisz, zmień etykietę na slajdzie 07 (tekst `PWG · wczesna wersja · ta sama kamera`) i zdanie w notatkach. |
| `mask-elev.png`, `mask-mountain.png`, `mask-height.png`, `mask-final.png` | 08 (`mask-final` także w tle siatki na 11) | Prawdziwe podglądy TerrainDebug ze starszej wersji (passy Elevation / MountainMask / MountainHeight / Final), 208 px, przeskalowane. | Podglądy masek z aktualnego grafu (np. Noise Height, Noise Mask, Slope Mask, Output Height), kwadratowe, min. 480 × 480, w skali szarości. Przy zmianie nazw nodów zaktualizuj podpisy na slajdzie 08. |

## Priorytet średni (slajdy działają na ilustracjach poglądowych)

| Plik (nowy) | Slajd | Co nagrać |
|---|---|---|
| `chunks-debug.jpg` | 12 | Widok z góry z zaznaczonymi granicami chunków (jeśli tryb debug je rysuje) albo wireframe całej mapy. Wymaga małej zmiany w HTML: dodaj `<img class="bgimg" src="assets/pwg/chunks-debug.jpg">` w `.s12 .map` zamiast `<canvas>`. |
| `lod-wireframe.jpg` | 13 | Wireframe terenu (Debug → Wireframe) z daleka, na którym widać gęstsze chunki blisko kamery i rzadsze w oddali. |
| *(zrobione 20.10)* | 10 | Slajd pokazuje teraz prawdziwe nagranie symulacji erozji z cząstkami (`erosion-sim.mp4`). Opcjonalnie: dłuższe ujęcie od `Reset Erosion` do stanu końcowego, w którym zmiana jest bardziej widoczna. Wtedy podmień `erosion-sim.mp4` i `erosion-sim-poster.jpg`. |

## Dane, których prezentacja świadomie NIE zawiera

Slajd 15 mówi wprost, że te pomiary są planowane. Gdy będą gotowe, można dodać wykres:
- czas `Run Graph` dla kilku rozdzielczości,
- czas kroku erozji i generowania hydrologii,
- średni FPS / czas klatki dla wariantu 31 × 129 (3999 × 3999),
- test powtarzalności: porównanie dwóch heightmap z tego samego seeda.
