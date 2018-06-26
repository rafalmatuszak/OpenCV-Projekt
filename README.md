# Projekt zaliczeniowy - laboratorium Przetwarzania Obrazów i Sekwencji wideo w OpenCV

## Autor: Rafał Matuszak, Informatyka II stopień

## Temat: Detekcja upadku w obrazach z monitoringu (Human Fall detection based on video surveillance).

### Idea

Detekcja ludzkiego upadku ma szczególne znaczenie w opiece ambulatoryjnej. W wielu przypadkach osoby objęte taką opieką, są osobami starszymi lub cierpiącymi na schorzenia, mogące powodować utraty przytomności. Często również, upadek wynikać może z potknięcia się, osoba taka nie może zazwyczaj sama poradzić sobie z powstaniem. Z pomocą przychodzą systemy detekcji upadku, które pozwalają wykrywać takie przypadki i powiadamiać odpowiednie osoby/opiekunów.

### Podstawy teoretyczne

Podstawy algorytmu do detekcji ludzkiego upadku opisane są w:
1.  [A method for real-time detection of human fall from video](https://infoscience.epfl.ch/record/213641/files/06240925.pdf)
2.  [Video Surveillance for Fall Detection ](https://www.intechopen.com/books/video-surveillance/video-surveillance-for-fall-detection)

Obydwa materiały opsiują detekcję updaku w następujący sposób: czytając kolejno klatki analizowanego obrazu, dokonujemy ekstrakcji największego przemieszczającego się w tym kadrze obiektu (w tym przypadku ludzkiej sylwetki). Można tego dokonać na kilka sposobów m.in. poprzez podstawową różnicę międzyklatkową lub z użyciem mieszanin gaussowskich (MOG). 
Zbadany ruch wykorzystywany będzie dalej do wyznaczenia tzw. współczynnika ruchu (ang. *motion coefficient*). Aby go obliczyć, oprócz różnicy w klatkach, musimy znać przebieg historii ruchu danego obiektu. W tym celu wyznaczany jest macierz MHI (ang. *Motion History of Image*), która reprezentuje historię ruchu obrazu w zadanym interwale czasowym. Sam wspólczynnik ruchu obliczany jest jako stosunek historii ruchu do wykrywanego obiektu pierwszoplanowego.

Kontur wydobytej wcześniej sylwetki można opisać za pomocą elipsy, której zmiany będa stanowiły podstawę do analizy ruchu obserwowanego człowieka i możliwości jego upadku. Brane pod uwagę będą takie czynniki jak:
* osie (mała i duża) kreślonej wokół człowieka elipsy,
* centrum elipsy (jako punkt),
* kąt nachylenia elipsy (podany w stopniach)

Same jednak wartości nie opsisują w czasie zmian badanej sylwetki. W tym celu, na bieżąco, należy wyznaczać wartości odchyleń standardowych dla:
* kąta,
* stosunku krótszej osi do dluższej osi,
* współrzędnej y środka elipsy.

Podane powyżej zależności wiąże poniższy schemat:

![Schemat](https://github.com/rafalmatuszak/OpenCV-Projekt/blob/master/opencv_project/Schemat.PNG)

W dużym uproszczeniu: sprawdzamy na początku wartość współczynnika ruchu. Jeśli jest on duży, analizujemy wykrytą sylwetkę przez sprawdzenie odchyleń standardowych kątą i stosunku a/b elipsy. Jeśli i te wartości są wysokie, należy sprawdzić czy ostatni parametr, odchylenie standardowe współrzędnej y środka elipsy, jest wysoki. Jeśli tak ( i spełnione są poprzednie warunki), można stwierdzić, że wykryto upadek.

### Środowisko realizacji

Powyższy projekt wykonano w języku C++, przy użyciu biblioteki OpenCV. Biblioteka ta, co warto zauważyć, choć ma bogaty zestaw algorytmow i bibliotek do operowania na obrazach, nie zawierała pewnych elementow biblioteki do badania przeplywu optycznego (OptFlow), który wykorzystywany jest do wyznaczania wspoomnianej wczesniej histori ruchu sekwencji. Autorzy OpenCV udostepnili jednak te (i wiele innych) dodatkowy moduly, nazywajac je **opencv_contrib** (ktore mozemy znalezc na repozytorium projektu  - [tutaj](https://github.com/opencv/opencv_contrib)).

W celu "ulepszenia" pierwotnej biblioteki OpenCV, autor projektu, wspierając się licznymi poradnikami znalezionymi w sieci i wspomnianym wczesniej repozytorium, skompilowal wlasna wersje biblioteki OpenCV, zawierajaca bogaty zakres dodatkowych modulow, m.in.:
* OptFlow,
* Text,
* Fuzzy image processing,
i wiele innych. 

### Realizacja

Sposob implementacji algorytmu zostal przedstawiony w pliku `Source.cpp`.

Jak mozna zauwazyc, glowny trzon algorytmu osadzony jest w funkcji `main`. W niej odbywają sie kolejno:
1. podstawowe operacje morfologiczne na obrazie,
2. utworzenie maski `pMOG2` (wyznaczamy w ten sposob obraz pierwszoplanowy - poruszającą sie postać),
3. obliczenie MHI (Motion History Image) - historii przebiegu ruchu,
4. obliczenie `mot_coeff` - wspolczynnika ruchu,
5. wyznaczenie konturów w obrazie oraz wyłuskanie największego konturu (ludzkiej sylwetki),
6. na podstawie wektora konturów, wyznaczenie (od drugiej klatki) odchylenia standardowego kata elipsy, stosunku a/b oraz wspolrzednej y srodka elipsy,
7. przekazanie wartosci `mot_coeff` oraz wartosci odchyleń standardowych do funkcji `fallDetect`, która sprawdza warunki algorytmu i zwraca wartosć logiczną `true`, gdy wykryto upadek lub `false`, gdy upadku nie ma

W celu lepszej obserwacji obliczanych wartosci, wiekszosc wyznaczanych danych wyswietlana jest na bieżąco na obrazie oraz zapisywana do pliku CSV `stddevs2.csv`. Umożliwiło to autorowi obserwację poprawnosci obliczanych parametrow.

### Wnioski

Zrealizowany projekt bada zachowanie elipsy, kreslonej  wokół sylwetki człowieka z sekwencji wideo. Na podstawie tego badania (wyznaczonych wartosci odchylen standardowych i współczynnika ruchu), stwierdza on, czy wykryto upadek.
Ze względu na trudnosci związane ze znalezieniem dobrego materiału wideo do analizy oraz niemożnosci zrealizowania własnego nagrania, autor dokonywał testów przede wszystkim na jednej sekwencji, dobrze realizującej założenia zadania. Mimo tego jednak, współczynniki odniesienia (odchylenia i współczynnik ruchu), do których porównywane są badane wartosci, odbiegaja od proponowanych przez autorów artykułów podanych powyżej. Wynikać to może z braku normalizacji lub niedostatecznego przetworzenia wstepnego obrazu.
Dlatego też, wartosci odniesienia zostały wyznaczone empirycznie, na podstawie obserwacji.
