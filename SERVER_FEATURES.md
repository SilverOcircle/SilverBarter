# Server Feature Liste

## SilverBarter - Tauschhandel System

Ein realistisches Barterhandel-System ohne Spielwährung. Spieler handeln direkt mit NPC-Händlern, die ihr eigenes dynamisches Inventar verwalten.

---

### Kernfeatures

| Feature | Beschreibung |
|---------|-------------|
| **Kein Geld / Keine Währung** | Pures Item-gegen-Item Tauschsystem - kein Exploit durch Geldfarming möglich |
| **Dynamischer Händlerbestand** | Jeder Händler hat sein eigenes Inventar, das sich durch Spielertrades verändert |
| **Angebot & Nachfrage** | Preise passen sich automatisch an den Lagerbestand an - voller Bestand = günstigere Preise |
| **Serverseitige Validierung** | Alle Berechnungen und Prüfungen laufen auf dem Server (Anti-Cheat) |
| **Persistenter Bestand** | Händlerinventare werden automatisch gespeichert und überleben Serverneustarts |

---

### Handelssystem im Detail

- **NPC Händler** - Konfigurierbare NPC-Trader mit eigenem Aussehen und Standort
- **Trading UI** - Übersichtliche Benutzeroberfläche zum Handeln
- **Kommissionssystem** - Einstellbare Gebühren pro Händler und Item-Typ
- **Qualitätsbewertung** - Abgenutzte Items werden niedriger bewertet; beschädigte Items können nicht verkauft werden
- **Mengenbewertung** - Teilweise gefüllte Stacks/Magazine werden anteilig bewertet
- **Limitierte Items** - Seltene Gegenstände mit kontrollierter Verfügbarkeit pro Restart
- **Kauf-/Verkaufsfilter** - Händler können auf bestimmte Item-Kategorien spezialisiert werden

---

### Sicherheit & Anti-Cheat

- Serverseitige Preisberechnung und Validierung
- Besitzprüfung - Spieler können nur eigene Items verkaufen
- Distanzprüfung - Spieler müssen in der Nähe des Händlers sein (5m)
- Tauschpflicht - Reines Abladen von Items ohne Gegenkauf ist blockiert
- Mengenlimits - Max. 10 verschiedene Item-Typen und 50 Items pro Typ pro Transaktion
- Bestandsvalidierung - Kann nicht mehr kaufen als der Händler auf Lager hat

---

### Konfiguration

- Beliebig viele Händler an verschiedenen Positionen
- Individuelles NPC-Aussehen (Kleidung, Ausrüstung)
- Eigene Kauf- und Verkaufsfilter pro Händler
- Kommissionsüberschreibungen für bestimmte Items (z.B. niedrigere Gebühren für seltene Items)
- Einstellbare Dumping-Parameter für Angebot/Nachfrage-Kurve
- Standard-Inventar für neue Händler konfigurierbar
- Debug-Modus für Fehlersuche

---

### Technische Details

| Parameter | Beschreibung |
|-----------|-------------|
| **Speicherintervall** | Automatische Sicherung alle 5 Minuten + bei Serverabschaltung |
| **Kompatibilität** | DayZ Stable - keine zusätzlichen Abhängigkeiten |
| **Autor** | [SilverOcircle](https://github.com/SilverOcircle) |
| **Basierend auf** | Syberia Project von Terje Bruoygard |

---

### Credits

Basierend auf dem Werk von **Terje Bruoygard** und seinem **Syberia Project**. Vielen Dank für die ursprüngliche Inspiration und Codebasis.

---

*Mod-Repository: [SilverBarter auf GitHub](https://github.com/SilverOcircle/SilverBarter)*
