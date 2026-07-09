# ESP32 RadarOS 📡

## 📖 O projekcie
Projekt **RadarOS** został zrealizowany w ramach kursu systemu operacyjnego czasu rzeczywistego **FreeRTOS** na studiach. Jego głównym celem jest demonstracja wielozadaniowości, zarządzania sprzętem oraz komunikacji między procesami w systemach wbudowanych. 

Urządzenie działa jako aktywny radar skanujący otoczenie w promieniu 180 stopni. System w czasie rzeczywistym analizuje odległość od przeszkód, a w przypadku wykrycia obiektu poniżej ustalonego progu bezpieczeństwa (regulowanego dynamicznie przez użytkownika), zatrzymuje skanowanie, uruchamia alarm dźwiękowy i wyświetla odpowiednie powiadomienia na ekranie LCD. Całość oparta jest na niezależnych zadaniach (Tasks) zarządzanych przez FreeRTOS, które komunikują się ze sobą bezkolizyjnie za pomocą kolejek (Queues) i grup zdarzeń (Event Groups).

---

## 🛠️ Wykorzystane komponenty

Poniżej znajduje się lista sprzętu wykorzystanego do budowy radaru:

* **Mikrokontroler ESP32** – "Mózg" całego systemu, obsługujący system FreeRTOS.
<br>![ESP32](Images/esp32.png)

* **Serwomechanizm SG90** – Odpowiada za płynny obrót czujnika w zakresie 0-180°.
<br>![Serwomechanizm](Images/servomotor.png)

* **Czujnik Ultradźwiękowy HC-SR04** – Dokonuje precyzyjnych pomiarów odległości od przeszkód.
<br>![Czujnik Ultradźwiękowy](Images/ultrasonic.png)

* **Potencjometr Analogowy** – Służy do płynnej regulacji progu zasięgu alarmu (od 1 cm do 100 cm).
<br>![Potencjometr](Images/potencjometr.png)

* **Buzzer (Active)** – Sygnalizator akustyczny informujący o wykryciu obiektu w strefie.
<br>![Buzzer](Images/buzzer.png)

* **Wyświetlacz LCD TFT ST7735 (128x160, SPI)** – Graficzny interfejs użytkownika (UI) oparty na bibliotece LVGL, wyświetlający aktualny status radaru.
<br>![Wyświetlacz LCD](Images/lcd.png)

---

## 🔌 Pinout (Połączenia sprzętowe)

Wszystkie peryferia zostały podłączone do mikrokontrolera ESP32 zgodnie z poniższą tabelą:

| Komponent | Pin Komponentu | Pin ESP32 (GPIO) | Opis / Funkcja |
| :--- | :---: | :---: | :--- |
| **Servo SG90** | PWM (Signal) | `GPIO 25` | Sygnał PWM (LEDC) sterujący kątem obrotu. |
| **HC-SR04** | TRIG | `GPIO 26` | Wyzwalanie impulsu dźwiękowego. |
| **HC-SR04** | ECHO | `GPIO 27` | Odbiór czasu powrotu impulsu. |
| **Potencjometr**| OUT / Wycieraczka | `GPIO 35` | Wejście ADC (ADC1_CH7) do odczytu progu. |
| **Buzzer** | VCC / Signal | `GPIO 33` | Cyfrowe wyjście wyzwalające alarm. |
| **LCD ST7735** | SCK / CLK | `GPIO 18` | Zegar magistrali SPI (SPI2_HOST). |
| **LCD ST7735** | SDA / MOSI | `GPIO 23` | Linia danych magistrali SPI. |
| **LCD ST7735** | CS | `GPIO 5` | Chip Select. |
| **LCD ST7735** | DC / RS | `GPIO 2` | Data/Command - wybór rejestru. |
| **LCD ST7735** | RST | `GPIO 4` | Reset sprzętowy ekranu. |
| **LCD ST7735** | BLK / LED | `3.3V` | Podświetlenie matrycy (na stałe). |

*(Zasilanie wszystkich komponentów odbywa się z linii 3.3V oraz 5V bezpośrednio z układu. Masa wszystkich układów jest połączona ze wspólnym GND).*

---

## 🧩 Architektura Oprogramowania (FreeRTOS)

Projekt został podzielony na modułowe pliki `.c` i `.h`, z których każdy ma ściśle określoną odpowiedzialność w systemie operacyjnym:

* **`main.c`** Główny plik startowy. Inicjalizuje mechanizmy komunikacji komunikację i uruchamia wszystkie poszczególne zadania (Tasks).

* **`Communication.c`** Rdzeń wymiany danych w systemie FreeRTOS. Zawiera definicje i inicjalizację kolejek (`angleQueue`, `thresholdQueue`, `distanceQueue`) oraz grupy zdarzeń (`systemEventGroup`), pozwalając na bezkolizyjną komunikację między zadaniami sprzętowymi a ekranem.

* **`ServoMotor.c`** Obsługuje sygnał PWM (używając sprzętowego timera LEDC) dla serwomechanizmu. Posiada wbudowany mechanizm natychmiastowego wstrzymania obrotu w przypadku zapalenia się flagi alarmu w grupie zdarzeń. Nadaje swój aktualny kąt do kolejki.

* **`UltrasonicSensor.c`** Odpowiada za wyzwalanie i odczyt czujnika odległości. Analizuje wynik pomiaru na tle progu z potencjometru. W zależności od sytuacji ustawia lub czyści flagę `BIT_ALARM_ON` w systemie.

* **`Potentiometer.c`** Korzysta z przetwornika ADC (OneShot) do odczytu napięcia z potencjometru. Mapuje surową wartość (0-4095) na centymetry (1-100) i wysyła ją jako próg alarmu do systemu.

* **`Alarm_Buzzer.c`** Zadanie uśpione, które nasłuchuje zdarzeń. Wybudza się i generuje przerywany sygnał dźwiękowy wyłącznie wtedy, gdy układ wykryje aktywną flagę alarmu, nie blokując przy tym zasobów procesora.

* **`st7735_lcd.c`** Zarządza magistralą SPI oraz potężnym silnikiem graficznym **LVGL**. Zadanie to działa jako pasywny "konsument" danych – asynchronicznie podgląda (`xQueuePeek`) wartości w kolejkach i rysuje na bieżąco interfejs użytkownika z zastosowaniem zmiennych kolorów dla alertów.

---

## 📸 Działanie projektu w praktyce

*(Wkrótce zostaną tu dodane zdjęcia oraz materiały wideo prezentujące urządzenie podczas skanowania i wykrywania obiektów!)*