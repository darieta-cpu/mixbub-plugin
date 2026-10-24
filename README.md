# MasterChain (v0.1)
Catena master: Filtri HPF/LPF -> EQ Pultec -> Compressore bus (SSL) -> Limiter lookahead.

## Build su GitHub (consigliato)
Fai push del progetto su un repository: Actions > "Build MasterChain (macOS)" parte da solo
(o con "Run workflow"). A fine build scarichi l'artifact `MasterChain-macOS` con VST3 e AU
(universal: Apple Silicon + Intel).

Installazione sul Mac:
    unzip MasterChain-VST3.zip -d ~/Library/Audio/Plug-Ins/VST3/
    xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/MasterChain.vst3
Poi in Reaper: Preferences > Plug-ins > VST > Re-scan.
(AU: stessa cosa con ~/Library/Audio/Plug-Ins/Components/MasterChain.component)

## Build locale (opzionale)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
JUCE viene scaricato da GitHub, oppure usa una cartella `JUCE/` locale / `-DJUCE_PATH=...`.

## Stato
- DSP testato da riga di comando (filtri, compressore, limiter, EQ).
- Parte JUCE (processor, parametri) non ancora compilata: se il build dà errori, mandameli.
- GUI custom (Source/PluginEditor.*, Source/UI/*) con meter di input, GR compressore e GR limiter.
- Limiter senza true peak/oversampling (passo successivo).
