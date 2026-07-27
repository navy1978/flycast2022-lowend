# Revisione manuale candidate Flycast 2022 Low-End

Cartella sul dispositivo:

```text
/storage/retrorun-test/flycast2022-candidates-review
```

Tutti i core usano lo stesso `retrorun.cfg`. Lo script esegue il reset DRM
completo prima di ogni prova e riavvia EmulationStation all'uscita.

## Avvio

Da SSH:

```sh
cd /storage/retrorun-test/flycast2022-candidates-review
./run-candidate.sh baseline "/storage/roms/dreamcast/Sonic Adventure 2.cdi"
./run-candidate.sh subimage "/storage/roms/dreamcast/Sonic Adventure 2.cdi"
```

Candidate disponibili:

| Nome | Stato tecnico | Cosa controllare |
| --- | --- | --- |
| `baseline` | riferimento accurato | immagine, audio e timing di confronto |
| `div1` | prestazioni respinte | gameplay, FPU/FPSCR, timing; non sono attese differenze visive |
| `shadow` | risultato incostante | animazioni texture, palette, mipmap, VQ, transizioni e RTT |
| `subimage` | +7,45% medio su MVC2 | texture dinamiche, menu, palette, RTT, caricamenti e cambi scena |
| `pal4` | impatto troppo piccolo | colori palette 4-bit, sprite/UI 2D e transizioni palette |

Non confrontare due candidate abilitate insieme: ogni `.so` contiene una sola
modifica. La baseline ha tutte le candidate disabilitate.

## Matrice minima

Per ogni core annotare:

- gioco, regione e product number;
- scena e durata;
- menu/overlay;
- trasparenze e punch-through;
- texture 2D e palette;
- render-to-texture/specchi/schermi;
- mipmap e filtraggio;
- audio;
- caricamento del save state;
- uscita e riavvio del core;
- esito: corretto, dubbio o regressione.

Priorità suggerita:

1. Marvel vs. Capcom 2: `baseline`, poi `subimage`, `shadow`, `pal4`;
2. Sonic Adventure 2: `baseline`, poi `subimage`;
3. Soul Calibur: `baseline`, poi `subimage`;
4. un gioco con render-to-texture e uno con texture mipmapped/VQ.

`SHA256SUMS` identifica esattamente i binari installati.

## Esito

La candidata `subimage` ha superato tutti i test visivi e uditivi eseguiti
dall'utente sulla RG351V. La decisione finale è **Keep**. L'integrazione la
abilita per impostazione predefinita e mantiene un'opzione runtime per tornare
al percorso texture originale.
