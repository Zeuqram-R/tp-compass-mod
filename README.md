# TP Compass

Brújula estilo Elden Ring para [Dusklight](https://github.com/TwilitRealm/dusklight) (Twilight Princess).

## Fase 1 (actual)
- Barra de brújula arriba de la pantalla con puntos cardinales (N, NE, E, SE, S, SO, O, NO)
- Marcas propias (diamante blanco) en tu posición actual, guardadas por partida
- Se oculta sola en cinemáticas, diálogos y menús
- Opciones: opacidad, ancho, posición, ángulo visible
- Calibración (dirección del norte, invertir) y panel de debug

## Cómo se instala en iOS
Los mods nativos en iOS tienen que ir dentro de la app. Cada push a `main` dispara
`.github/workflows/build-ios.yml`, que compila Dusklight v2.0.3 con el mod incluido y publica
un `.ipa` en *Releases*. Ese IPA se firma e instala con Signulous.

## Compilar el `.dusk` (PC / Mac / Linux)
```
cmake -B build -DDUSK_DIR=/ruta/a/dusklight   # checkout del tag v2.0.3
cmake --build build                            # -> build/mods/tp_compass.dusk
```
