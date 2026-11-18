# TP Compass

Brújula estilo Elden Ring para [Dusklight](https://github.com/TwilitRealm/dusklight) (Twilight Princess).

## Funciones
- Barra de brújula arriba de la pantalla con puntos cardinales (N, NE, E, SE, S, SO, O, NO)
- Marcas propias (diamante blanco) en tu posición actual, guardadas por partida
- Atajo de control para poner/quitar marcas (por defecto Z + cruceta izquierda)
- Cofres sin abrir (cuadro café/dorado), opcionalmente solo con la brújula del dungeon
- Nombre del lugar abajo a la izquierda (tabla de nombres de Dusklight, CC0)
- Se oculta sola en cinemáticas, diálogos y menús
- Opciones: opacidad, ancho, posición, ángulo visible
- Calibración (dirección del norte, invertir) y panel de debug

## Cómo se instala en iOS
Los mods nativos en iOS tienen que ir dentro de la app. El IPA se arma en
`Zeuqram-R/dusklight-ios-bundle`, que incluye este mod junto con los demás.

## Compilar el `.dusk` (PC / Mac / Linux)
```
cmake -B build -DDUSK_DIR=/ruta/a/dusklight   # checkout del tag v2.0.3
cmake --build build                            # -> build/mods/tp_compass.dusk
```
