# Mixbus Chain (v0.1) - DaniArieta
Catena master: Filtri HPF/LPF -> EQ Pultec -> Compressore bus (SSL) -> Limiter lookahead.

## Build su GitHub (consigliato)
Fai push del progetto su un repository: Actions > "Build Mixbus Chain" parte da solo
(o con "Run workflow"). A fine build scarichi l'artifact `MixbusChain-macOS` con VST3 e AU
(universal: Apple Silicon + Intel).

Installazione sul Mac:
    unzip MixbusChain-VST3.zip -d ~/Library/Audio/Plug-Ins/VST3/
    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/Mixbus Chain.vst3"
Poi in Reaper: Preferences > Plug-ins > VST > Re-scan.
(AU: stessa cosa con "~/Library/Audio/Plug-Ins/Components/Mixbus Chain.component")

## Windows
Lo stesso workflow produce anche l'artifact `MixbusChain-Windows` (VST3 a 64 bit).
Estrai lo zip: dentro c'e' la cartella `Mixbus Chain.vst3`. Copiala in
`C:\Program Files\Common Files\VST3\` (servono i permessi di amministratore), oppure in una
cartella tua da aggiungere in Reaper: Options > Preferences > Plug-ins > VST > Edit path list.
Poi Re-scan.

## Build locale (opzionale)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
JUCE viene scaricato da GitHub, oppure usa una cartella `JUCE/` locale / `-DJUCE_PATH=...`.

## Sezione nastro (TAPE)
Dopo l'input (o, con l'interruttore Position, subito prima del limiter): saturazione asimmetrica con
oversampling 2x, head bump, perdita di alte (Tone), rolloff che dipende dalla velocita' (7.5/15/30 ips),
Mix parallelo e Output trim. Il modulo introduce 33 campioni di latenza fissa (anche se spento), che si
sommano a quella del limiter e vengono comunicati al DAW. Nessun wow/flutter ne' rumore (per ora).

## Stato
- DSP testato da riga di comando (filtri, compressore, limiter, EQ).
- Parte JUCE (processor, parametri) non ancora compilata: se il build dà errori, mandameli.
- GUI custom (Source/PluginEditor.*, Source/UI/*) con meter di input, GR compressore e GR limiter.
- Limiter senza true peak/oversampling (passo successivo).
- Finestra ridimensionabile (trascina l'angolo in basso a destra).
