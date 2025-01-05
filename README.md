# Projekt: Pociąg z Rowerami

---

## Opis projektu
Symulacja działania systemu zarządzania pociągiem pasażerskim, który przewozi pasażerów oraz rowery. Projekt jest realizowany w ramach zajęć z systemów operacyjnych i skupia się na zastosowaniu mechanizmów IPC (Inter-Process Communication) oraz synchronizacji procesów w systemie Linux.

---

## Funkcjonalności
### Zawiadowca
- Zarządza przyjazdem i odjazdem pociągów.
- Wysyła sygnały do pasażerów i kierownika pociągu.
- Obsługuje sytuacje, gdy pociąg stoi zbyt długo lub jest pełny.

### Kierownik pociągu
- Zarządza załadunkiem pasażerów i rowerów.
- Obsługuje pasażerów na podstawie kolejek komunikatów.
- Obsługuje sygnały do pominięcia załadunku (SIGUSR1).

### Pasażerowie
- Mogą mieć rower (30% szans na rower).
- Wchodzą na peron i wysyłają komunikaty do kierownika pociągu.
- Obsługują sygnały SIGUSR2 (wrót do poczekalni).

---

## Struktura projektu

### Pliki
- **`zawiadowca.c`**: Kod odpowiedzialny za zarządzanie pociągami na stacji.
- **`kierownik.c`**: Kod kierownika zarządzającego załadunkiem pasażerów i rowerów.
- **`pasazer.c`**: Kod reprezentujący proces pasażera.
- **`mojeFunkcje.h`**: Zestaw funkcji pomocniczych do zarządzania IPC i synchronizacją.

### Komunikacja między procesami
- **Kolejki komunikatów**: Zarządzanie komunikacją między zawiadowcą, kierownikiem i pasażerami.
- **Semafory**: Synchronizacja dostępu do zasobów (platformy, wejścia do pociągu).
- **Sygnały**: Obsługa zdarzeń, takich jak pominięcie załadunku czy cofnięcie pasażerów do poczekalni.

---

## Diagram architektury

```mermaid
graph TD
    subgraph Zawiadowca
        A[Oczekiwanie na pociąg]
        B[Wysyłanie sygnałów]
        C[Koordynacja semaforów]
    end

    subgraph Kierownik
        D[Załadunek pasażerów]
        E[Obsługa rowerów]
        F[Zezwolenie na odjazd]
    end

    subgraph Pasażerowie
        G[Oczekiwanie na platformie]
        H[Wchodzenie do pociągu]
        I[Obsługa sygnałów]
    end

    A -->|Kolejka komunikatów| D
    D -->|Sygnał| G
    H -->|Powiadomienie| F
    B -->|SIGUSR2| I
    I -->|Zwrot na platformę| G
