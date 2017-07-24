# WemosD1 HomeMatic Helligkeitssensor

## Zusammenfassung:
Der Sensor übermittelt in einem konfigurierbaren Intervall den Helligkeitswert (ca. 0...1023) an eine Variable der CCU.
_(Genauere Werte sind mit einer Photodiode statt eines Photowiderstands (LDR) erreichbar, jedoch hatte ich zum Zeitpunkt des Projekts noch einige LDR zu liegen.)_



## Teileliste:
- 1x [Wemos D1 mini](http://www.ebay.de/itm/272271662681)
- 1x Stromversorgungsmodul, z.B. [HLK-PM01](http://www.ebay.de/itm/272521453807)
- 1x [LDR](http://www.ebay.de/itm/321957950526)
- 1x [Widerstand 10k](http://www.ebay.de/itm/221833069520)
- 1x [Taster](http://www.ebay.de/itm/263057910534), wird jedoch nur zur (Erst-)Konfiguration benötigt
(_statt des Tasters kann man auch mit einer temporären Drahtbrücke arbeiten_) 

## Verdrahtung:
![wiring](Images/wiring.png)

(Verdrahtung)

![Beispielaufbau](Images/beispielaufbau.JPG)

(Beispielaufbau (ohne Taster!))



## Konfiguration des Wemos D1
Um den Konfigurationsmodus zu starten, muss der Wemos D1 **mit gedrückt gehaltenem Taster gestartet** werden.
Die **blaue LED am Wifi-Modul blinkt kurz und leuchtet dann dauerhaft. **

**Der Konfigurationsmodus ist nun aktiv.**

Auf dem Handy oder Notebook sucht man nun nach neuen WLAN Netzen in der Umgebung. 

Es erscheint ein neues WLAN mit dem Namen "WemosD1-xx:xx:xx:xx:xx:xx"

Nachdem man sich mit diesem verbunden hat, öffnet sich automatisch das Konfigurationsportal.

Geschieht dies nicht nach ein paar Sekunden, ist im Browser die Seite http://192.168.4.1 aufzurufen.

**WLAN konfigurieren auswählen**

**SSID**: WLAN aus der Liste auswählen, oder SSID manuell eingeben

**WLAN-Key**: WLAN Passwort

**CCU IP**: selbsterklärend

**Variablenname**: Name der Variable, die in der CCU angelegt wurde

**Übertragung alle x Sekunden**: Sende-Intervall
