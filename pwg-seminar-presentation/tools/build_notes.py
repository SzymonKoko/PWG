"""Regenerate SPEAKER_NOTES.md from the <aside class="notes"> blocks in index.html.

Usage (from the pwg-seminar-presentation folder):
    python tools/build_notes.py
"""
import html
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
src = (ROOT / "index.html").read_text(encoding="utf-8")

slides = re.findall(
    r'<section class="slide[^"]*" data-title="([^"]+)".*?<aside class="notes" data-time="(\d+)">(.*?)</aside>',
    src, re.S)


def clean(fragment):
    return html.unescape(re.sub(r"<[^>]+>", "", fragment).strip())


rows, body, total = [], [], 0
for i, (title, seconds, notes) in enumerate(slides):
    seconds = int(seconds)
    total += seconds
    title = html.unescape(title)
    paras = re.findall(r'<p( class="tr")?>(.*?)</p>', notes, re.S)
    narration = [clean(p) for c, p in paras if not c]
    transition = [clean(p).replace("Przejście: ", "") for c, p in paras if c]
    rows.append(f"| {i:02d} | {title} | {seconds} s |")
    body += [f"## {i:02d} · {title}", "", f"**Czas:** ok. {seconds} s", "", "**Narracja:**", ""]
    body += [p + "\n" for p in narration]
    body += ["**Przejście:** " + (transition[0] if transition else "— (koniec prezentacji, pytania)"), "", "---", ""]

header = [
    "# Notatki prelegenta — „Jak stworzyć świat, który nie istnieje?”",
    "",
    "Seminarium dyplomowe · *Proceduralne generowanie świata na potrzeby gier i symulacji. Analiza algorytmów.*",
    "",
    "Plik generowany z notatek w `index.html` (`<aside class=\"notes\">`) skryptem `tools/build_notes.py`. "
    "Te same notatki są dostępne w prezentacji pod klawiszem **N** (panel pod slajdem) i **S** (osobne okno prowadzącego z zegarem).",
    "",
    "`[Klik]` oznacza kolejne naciśnięcie → / spacji, które odsłania następny element slajdu. "
    "Slajd 00 to plansza tytułowa wyświetlana przed rozpoczęciem; slajdy 01–17 odpowiadają planowi prezentacji.",
    "",
    "## Plan czasowy",
    "",
    "| # | Slajd | Czas |",
    "|---|---|---|",
    *rows,
    f"| | **Razem** | **{total} s ≈ {total / 60:.1f} min** |",
    "",
]
(ROOT / "SPEAKER_NOTES.md").write_text("\n".join(header + body), encoding="utf-8")
print(f"{len(slides)} slajdów, łącznie {total} s ({total / 60:.1f} min)")
