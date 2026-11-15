# Notatki prelegenta — „Jak stworzyć świat, który nie istnieje?”

Seminarium dyplomowe · *Proceduralne generowanie świata na potrzeby gier i symulacji. Analiza algorytmów.*

Plik generowany z notatek w `index.html` (`<aside class="notes">`) skryptem `tools/build_notes.py`. Te same notatki są dostępne w prezentacji pod klawiszem **N** (panel pod slajdem) i **S** (osobne okno prowadzącego z zegarem).

`[Klik]` oznacza kolejne naciśnięcie → / spacji, które odsłania następny element slajdu. Slajd 00 to plansza tytułowa wyświetlana przed rozpoczęciem; slajdy 01–17 odpowiadają planowi prezentacji.

## Plan czasowy

| # | Slajd | Czas |
|---|---|---|
| 00 | Tytuł | 15 s |
| 01 | Kto stworzył te światy? | 45 s |
| 02 | Jeden algorytm. Miliony możliwości. | 40 s |
| 03 | Nie tylko gry. | 45 s |
| 04 | Czas rzeczywisty. | 45 s |
| 05 | Procedural World Generator | 45 s |
| 06 | Od liczb do krajobrazu. | 55 s |
| 07 | Zmieniasz parametry. Zmieniasz świat. | 45 s |
| 08 | Jeden szum nie wystarczy. | 55 s |
| 09 | Od operacji do pipeline'u. | 55 s |
| 10 | Natura zmienia teren. | 55 s |
| 11 | Kto wykonuje obliczenia? | 50 s |
| 12 | Chunki. | 45 s |
| 13 | LOD — Level of Detail. | 45 s |
| 14 | PWG w praktyce. | 70 s |
| 15 | Czy ten świat działa poprawnie? | 50 s |
| 16 | Świat nadal się rozwija. | 45 s |
| 17 | Nie stworzyłem jednej mapy. | 30 s |
| | **Razem** | **835 s ≈ 13.9 min** |

## 00 · Tytuł

**Czas:** ok. 15 s

**Narracja:**

Dzień dobry. Nazywam się [imię i nazwisko], a tematem mojej pracy inżynierskiej jest proceduralne generowanie świata na potrzeby gier i symulacji.

Za mną widzicie krajobraz, którego nikt nie wymodelował ręcznie — ale o tym za chwilę.

**Przejście:** Zanim pokażę, co zbudowałem, zacznę od pytania.

---

## 01 · Kto stworzył te światy?

**Czas:** ok. 45 s

**Narracja:**

Spójrzcie na te trzy światy: Minecraft, No Man's Sky i film Pixara „Dobry dinozaur”. Każdy z nich jest ogromny — i w każdym dużą część pracy wykonał nie grafik z myszką, tylko algorytm albo dane.

W Minecrafcie teren powstaje z ziarna losowości, czyli seeda, i funkcji szumu. W No Man's Sky w podobny sposób powstają całe planety.

A Pixar przy „Dobrym dinozaurze” zbudował krajobrazy na podstawie prawdziwych danych wysokościowych amerykańskiej służby geologicznej USGS.

Więc kto tak naprawdę stworzył te światy?

**Przejście:** Odpowiedź brzmi — w dużej mierze reguły.

---

## 02 · Jeden algorytm. Miliony możliwości.

**Czas:** ok. 40 s

**Narracja:**

Ręczne wymodelowanie jednej doliny to dni pracy grafika. Tysiąca dolin — lata.

Generowanie proceduralne odwraca ten problem: zamiast tworzyć każde miejsce osobno, tworzymy reguły, które te miejsca produkują.

[Klik] No Man's Sky to najbardziej znany przykład: według twórców gra może wygenerować ponad 18 trylionów planet — dokładnie 2 do potęgi 64, bo każdą planetę wyznacza jej ziarno, czyli seed. Żadna z nich nie jest zapisana na dysku; każda jest wyliczana z algorytmu w chwili, gdy jest potrzebna. Studio Hello Games opowiadało o tej architekturze na GDC 2017.

**Przejście:** I to nie jest wyłącznie domena gier.

---

## 03 · Nie tylko gry.

**Czas:** ok. 45 s

**Narracja:**

Kino też korzysta z tych technik. Przy „Królu Lwie” z 2019 roku studio MPC odeszło od malowanych teł. Według materiałów studia powstało 66 cyfrowych planów o łącznej powierzchni około 150 kilometrów kwadratowych, budowanych proceduralnie na podstawie fotografii referencyjnych i reguł ekosystemu.

Z kolei w „Dobrym dinozaurze” teren pochodził z rzeczywistych map wysokości. To dokładnie ta sama struktura danych, której używa mój projekt — heightmapa.

[Klik] Jest jednak zasadnicza różnica: film renderuje się offline. Pojedyncza klatka może liczyć się minutami, a nawet godzinami na farmie renderującej — i nikt nie czeka na nią przed ekranem.

**Przejście:** Gra nie ma takiego luksusu.

---

## 04 · Czas rzeczywisty.

**Czas:** ok. 45 s

**Narracja:**

Gra działa w czasie rzeczywistym.

[Klik] Przy 60 klatkach na sekundę na wszystko — logikę gry, fizykę i narysowanie świata — mamy około 16,7 milisekundy.

Horizon Zero Dawn to dobry przykład kompromisu. Teren projektują tam artyści, ale roślinność, skały i wiele innych obiektów rozmieszcza w czasie rzeczywistym system proceduralny działający na karcie graficznej — Guerrilla opowiadała o nim na GDC 2017. Czyli procedury nie muszą generować wszystkiego; mogą przejąć to, co powtarzalne.

To pytanie — jak generować świat i jednocześnie zmieścić się w budżecie klatki — postanowiłem zbadać sam.

**Przejście:** Tak powstał mój projekt.

---

## 05 · Procedural World Generator

**Czas:** ok. 45 s

**Narracja:**

Tak powstał PWG — Procedural World Generator. To autorski silnik i edytor napisany od zera w C++20 i OpenGL 4.5, bez Unity czy Unreal Engine.

To, co widzicie, to zrzut ekranu z silnika: kształt gór, jezioro, rzeki, materiały terenu i oświetlenie są wyliczane przez program.

Moim celem nie było narysowanie jednej ładnej mapy, tylko zbudowanie konfigurowalnego narzędzia, w którym taki świat projektuje się regułami.

**Przejście:** Pokażę krok po kroku, jak to działa — zaczynając od najbardziej podstawowego pytania.

---

## 06 · Od liczb do krajobrazu.

**Czas:** ok. 55 s

**Narracja:**

Jak z liczb powstają góry? Teren w PWG to mapa wysokości, czyli heightmapa: siatka, w której każda komórka przechowuje jedną liczbę — wysokość w tym punkcie.

[Klik] Te liczby nie są przypadkowe w zwykłym sensie. Pochodzą z funkcji szumu gradientowego, która daje płynne, naturalnie wyglądające zmiany. Jeśli zamienimy je na odcienie szarości, dostajemy obraz — jasne to wysoko, ciemne to nisko. W projekcie używam biblioteki FastNoiseLite, także w wersji GLSL działającej na karcie graficznej.

[Klik] Kiedy potraktujemy te wartości jako wysokości wierzchołków płaskiej siatki, płaszczyzna zamienia się w krajobraz. Uczciwie dodam: to model 2,5D — jedna wysokość na punkt — więc bez jaskiń i nawisów.

**Przejście:** Najciekawsze jest to, że tym krajobrazem można sterować.

---

## 07 · Zmieniasz parametry. Zmieniasz świat.

**Czas:** ok. 45 s

**Narracja:**

Funkcja szumu ma parametry: częstotliwość, liczbę oktaw, typ fraktala, amplitudę i ziarno, czyli seed.

Te trzy ujęcia pochodzą z wczesnej wersji PWG i są zrobione z tej samej kamery — zmieniałem wyłącznie parametry generatora. Ten sam algorytm daje łagodne wzgórza, ostre iglice albo zwarte pasmo górskie.

[Klik] Co ważne, generowanie jest deterministyczne: ten sam seed i te same parametry dają ten sam świat. Nie trzeba więc zapisywać całej mapy — wystarczy zapisać przepis.

**Przejście:** Ale jeden szum to wciąż za mało.

---

## 08 · Jeden szum nie wystarczy.

**Czas:** ok. 55 s

**Narracja:**

Jeden szum daje teren jednorodny — wszędzie podobny. Prawdziwy krajobraz ma regiony: niziny, pasma górskie, doliny. Dlatego PWG składa teren z warstw.

To są prawdziwe dane pośrednie z jednej z wcześniejszych wersji silnika, podglądane przez moduł TerrainDebug. Najpierw ogólne ukształtowanie terenu.

[Klik] Potem maska, która mówi, gdzie mają powstać góry. [Klik] Potem szczegółowa wysokość samych gór.

[Klik] Na końcu shader obliczeniowy łączy warstwy ważoną sumą i lekko podbija kontrast funkcją potęgową — tak powstaje finalna heightmapa.

**Przejście:** Kiedy takich operacji robi się kilkanaście, potrzebny jest porządek.

---

## 09 · Od operacji do pipeline'u.

**Czas:** ok. 55 s

**Narracja:**

Ten porządek zapewnia graf nodów — TerrainGraph. Każdy node to jedna operacja: generator szumu, maska, działanie matematyczne, symulacja albo wyjście.

Na ekranie jest uproszczony przykład z prawdziwymi nazwami nodów z edytora PWG: szum wysokości mnożony przez maskę, potem dwie symulacje erozji i wyjście, z którego powstaje teren 3D.

[Klik] Wynik jednego kroku staje się wejściem następnego. Zanim graf zostanie wykonany, silnik go waliduje — sprawdza cykle, brakujące wejścia i zgodność typów portów. W edytorze jest ponad trzydzieści typów nodów: od generatorów, przez kilkanaście operacji matematycznych i maski analizujące teren, po erozję, hydrologię i rozmieszczanie roślinności.

**Przejście:** Dwa z tych nodów są szczególne, bo to nie matematyka, tylko symulacja.

---

## 10 · Natura zmienia teren.

**Czas:** ok. 55 s

**Narracja:**

Szum daje kształt, ale nie daje historii. Prawdziwe góry są wyrzeźbione przez wodę i grawitację — mają żleby, osypiska i płaskie dna dolin.

[Klik] PWG symuluje to na dwa sposoby. Erozja hydrauliczna to tysiące cząstek wody liczonych na GPU: każda spływa po zboczu, wymywa materiał, przenosi go i odkłada niżej — szczyty się obniżają, a doliny wypełniają osadem. Erozja termiczna zsuwa materiał ze zboczy zbyt stromych, aż osiągną stabilny kąt.

[Klik] Obie symulacje są zaimplementowane i działają interaktywnie — to jednak uproszczone modele na mapie wysokości, a nie pełna symulacja fizyczna. Po prawej widać nagranie z PWG: symulacja działa na żywo, a białe punkty to cząstki wody, które spływają po zboczach i przenoszą materiał. Na tak ukształtowanym terenie osobny generator hydrologii wyznacza potem rzeki i jeziora.

**Przejście:** Takie symulacje to miliony obliczeń. Kto je wykonuje?

---

## 11 · Kto wykonuje obliczenia?

**Czas:** ok. 50 s

**Narracja:**

Heightmapa w największym wariancie PWG ma prawie 4000 na 4000 próbek, czyli około 16 milionów wartości. Gdyby procesor przeliczał je jedna po drugiej, każda zmiana parametru oznaczałaby czekanie.

[Klik] Ale te obliczenia są w dużej mierze niezależne — wartość w jednym punkcie nie czeka na sąsiada. To idealne zadanie dla karty graficznej, która ma tysiące prostych rdzeni.

[Klik] W PWG operacje grafu, maski, normalne i erozja działają jako compute shadery OpenGL 4.5, uruchamiane w grupach roboczych 8 na 8 wątków. Procesorowi zostaje to, co ma charakter sekwencyjny: edytor, hydrologia oparta na algorytmach grafowych i zarządzanie sceną.

**Przejście:** Policzyć teren to jedno. Trzeba go jeszcze narysować.

---

## 12 · Chunki.

**Czas:** ok. 45 s

**Narracja:**

Policzenie terenu to jedno, narysowanie go — drugie. Dlatego mapa jest podzielona na chunki, czyli kwadratowe fragmenty. W PWG można skonfigurować od 3 do 31 chunków na bok.

[Klik] Każdy chunk jest sprawdzany względem bryły widzenia kamery. Chunki poza nią w ogóle nie są rysowane — to frustum culling.

Ważne zastrzeżenie: PWG generuje świat o skończonym, z góry ustalonym rozmiarze. Chunki służą tu do wydajnego renderingu, a nie do nieskończonego strumieniowania świata.

**Przejście:** A co z chunkami, które są widoczne, ale bardzo daleko?

---

## 13 · LOD — Level of Detail.

**Czas:** ok. 45 s

**Narracja:**

Tu wchodzi LOD — level of detail, czyli poziom szczegółowości.

Pojedynczy chunk w pełnej rozdzielczości ma 128 na 128 kwadratów, czyli ponad 32 tysiące trójkątów. Kiedy chunk jest daleko, zajmuje na ekranie kilka pikseli — rysowanie wszystkich tych trójkątów byłoby marnotrawstwem.

[Klik] Dlatego LODManager w PWG ma pięć poziomów. Każdy kolejny ma dwa razy mniej wierzchołków na bok, czyli cztery razy mniej trójkątów — od 32 768 do zaledwie 128. Poziom jest wybierany na podstawie odległości chunka od kamery.

**Przejście:** Tyle teorii. Zobaczmy to w działaniu.

---

## 14 · PWG w praktyce.

**Czas:** ok. 70 s

**Narracja:**

To są nagrania z PWG, bez żadnych efektów dodanych w montażu. Najpierw timelapse przyspieszony dziesięciokrotnie: około trzech i pół minuty pracy w edytorze grafu. Zaczynam od płaskiej płaszczyzny, dokładam generatory szumu, operacje matematyczne i maski, a teren po każdym uruchomieniu grafu zmienia się na żywo.

Potem przelot kamery w trybie eksploracji po gotowych światach.

Najpierw górski świat: jezioro, którego poziom wyznaczył generator hydrologii, zbocza wyrzeźbione erozją, rzeki, wodospady i roślinność rozmieszczona proceduralnie.

Potem zupełnie inny świat — pustynia z wydmami, stworzona tym samym silnikiem. Na koniec widok całej mapy z góry. Wszystko, co widać, jest renderowane w czasie rzeczywistym.

**Przejście:** Ładny obraz to jednak za mało. Trzeba jeszcze wiedzieć, czy ten świat działa poprawnie.

---

## 15 · Czy ten świat działa poprawnie?

**Czas:** ok. 50 s

**Narracja:**

Jak ocenić generator świata? Proponuję trzy kryteria.

[Klik] Powtarzalność: ten sam seed powinien dać identyczną heightmapę. Algorytmy są deterministyczne z założenia, a formalny test porównujący wyniki jest zaplanowany.

[Klik] Poprawność: rzeki muszą płynąć w dół, a tafla jeziora musi być pozioma. Na razie sprawdzam to wizualnie i przez podgląd danych pośrednich w trybie debug. Metryki liczbowe — na przykład odchylenie poziomu tafli — są w planie.

[Klik] Wydajność: czas wykonania grafu, kroku erozji i generowania hydrologii oraz liczba klatek na sekundę dla różnych rozdzielczości. Te pomiary są częścią pracy, którą kończę — dlatego dziś nie pokazuję liczb, których jeszcze nie zmierzyłem.

**Przejście:** To prowadzi do tego, co dalej.

---

## 16 · Świat nadal się rozwija.

**Czas:** ok. 45 s

**Narracja:**

Projekt nadal się rozwija.

[Klik] Erozja: dziś graf obsługuje jeden node erozji hydraulicznej i jeden termicznej. Kolejny krok to łączenie wielu etapów symulacji, na przykład na różnych fragmentach mapy.

[Klik] Woda: rzeki i jeziora są wyznaczane jednorazowo, a ruch wody na powierzchni jest efektem wizualnym. Prawdziwym wyzwaniem byłaby dynamiczna symulacja przepływu.

[Klik] Optymalizacja: systematyczne pomiary i profilowanie, żeby wiedzieć, gdzie naprawdę jest wąskie gardło. Architektura grafu nodów pozwala dodawać takie elementy bez przebudowy całego silnika.

**Przejście:** Wróćmy na koniec do pytania z początku.

---

## 17 · Nie stworzyłem jednej mapy.

**Czas:** ok. 30 s

**Narracja:**

Na początku zapytałem, kto stworzył te światy. Odpowiedź, do której doszedłem, pisząc PWG, brzmi tak:

Nie stworzyłem jednej mapy. [pauza, klik — tło przechodzi z gór w pustynię, też wygenerowaną w PWG] Stworzyłem sposób na tworzenie wielu.

Dziękuję — chętnie odpowiem na pytania.

**Przejście:** — (koniec prezentacji, pytania)

---
