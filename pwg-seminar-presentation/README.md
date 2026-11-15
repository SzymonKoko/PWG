# PWG — prezentacja na seminarium dyplomowe

**„Jak stworzyć świat, który nie istnieje?”** — od matematyki do wirtualnego krajobrazu.

Prezentacja HTML 16:9 (scena 1920 × 1080 skalowana do okna): 17 slajdów plus plansza tytułowa, ok. 14 minut. Działa w całości offline, bez instalowania czegokolwiek.

## Uruchomienie

**Najprościej:** otwórz `index.html` w Chrome, Edge albo Firefoxie (dwuklik).

**Zalecane na projektor:** wideo i fonty najpewniej ładują się z lokalnego serwera:

```bash
python -m http.server 8000
```

Potem otwórz `http://localhost:8000` i naciśnij **F**.

Przed wystąpieniem przejdź raz całą prezentację, żeby przeglądarka zbuforowała obrazy i wideo.

## Wersja PDF

`PWG-prezentacja.pdf` ma 18 stron w formacie 1920 × 1080, po jednej na slajd. Każdy slajd jest w stanie końcowym: wszystkie kroki odsłonięte, bez animacji, a w miejscu filmów są ich klatki-plakaty.

Tę wersję daje tryb `index.html?print`. Żeby odtworzyć PDF po zmianach, uruchom w folderze prezentacji (Chrome lub Edge):

```bash
"C:/Program Files/Google/Chrome/Application/chrome.exe" --headless=new --no-pdf-header-footer --virtual-time-budget=20000 --print-to-pdf="PWG-prezentacja.pdf" "file:///C:/dev/Procedural%20World%20Generation/pwg-seminar-presentation/index.html?print"
```

Możesz też otworzyć `index.html?print` w przeglądarce i wydrukować do PDF (Ctrl+P, marginesy: brak, grafika tła: włączona).

## Sterowanie

| Klawisz | Działanie |
|---|---|
| → / Spacja / Enter / PageDown | następny element slajdu, a po ostatnim — następny slajd |
| ← / PageUp / Backspace | krok wstecz |
| Home / End | pierwszy / ostatni slajd |
| F | pełny ekran |
| N | notatki prelegenta pod slajdem |
| S | osobne okno prowadzącego: notatki, następny slajd, zegar (przeciągnij na laptopa, a prezentację na projektor) |
| P | odtwórz / zatrzymaj wideo (slajd 14) |
| B lub . | wygaszenie ekranu |
| E | tryb edycji tekstów (Ctrl+S pobiera zmieniony `index.html`) |
| ? | pomoc |

Działają też kółko myszy i przesunięcie palcem. Adres `index.html#7` otwiera wprost slajd 7, a `index.html#17.1` otwiera slajd 17 z odsłoniętym pierwszym krokiem.

Licznik w prawym dolnym rogu pokazuje `slajd / ostatni · krok / liczba kroków`.

## Struktura

```
pwg-seminar-presentation/
├── index.html          # cała prezentacja: HTML + CSS + JS w jednym pliku
├── assets/
│   ├── pwg/            # materiały z PWG (zrzuty, maski, pwg-demo.mp4)
│   ├── ref/            # kadry z gier i filmów (atrybucje w SOURCES.md)
│   └── fonts/          # Bricolage Grotesque + JetBrains Mono (woff2, lokalnie)
├── tools/build_notes.py  # generuje SPEAKER_NOTES.md z notatek w index.html
├── SPEAKER_NOTES.md    # pełna narracja po polsku, przejścia i plan czasowy
├── SOURCES.md          # źródła obrazów i stwierdzeń
└── ASSETS_NEEDED.md    # które materiały PWG warto nagrać ponownie
```

## Modyfikacje

- **Kolory i fonty:** zmienne w bloku `:root` na początku `<style>` (`--teal`, `--gold`, `--bg`…).
- **Tekst slajdu:** każdy slajd to `<section class="slide sNN">` z komentarzem `NN — tytuł`. Teksty można też poprawić w przeglądarce (klawisz **E**).
- **Notatki:** `<aside class="notes" data-time="45">` na końcu każdego slajdu (`data-time` to sekundy). Akapit `class="tr"` to przejście do następnego slajdu. Po zmianach uruchom `python tools/build_notes.py`.
- **Animacje:**
  - Element z klasą `rv` pojawia się sam po wejściu na slajd; opóźnienie ustawia `style="--d:.6s"`.
  - Element z klasą `step` pojawia się po kolejnym naciśnięciu →.
- **Obrazy:** podmień plik pod tą samą nazwą (patrz `ASSETS_NEEDED.md`). Brakujący plik pokazuje widoczny placeholder „BRAK PLIKU”.
- **Ruch:** przy włączonym „ograniczeniu ruchu” w systemie (`prefers-reduced-motion`) animacje są wyłączane, a cała treść zostaje widoczna.

## Uwagi merytoryczne

Twierdzenia o PWG pochodzą z kodu i `docs/PROJECT_DESCRIPTION.md` (stan 21.09.2026). Kilka rzeczy jest pokazanych świadomie ostrożnie:

- **Pomiary:** brak gotowych pomiarów wydajności, więc slajd 15 oznacza je jako planowane.
- **Chunki:** służą do renderingu skończonej mapy, a nie do nieskończonego strumieniowania (slajd 12).
- **Przykłady z przemysłu:**
  - Horizon Zero Dawn to proceduralne rozmieszczanie obiektów, nie terenu.
  - Dobry dinozaur korzystał z rzeczywistych danych USGS.
