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
