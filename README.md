# TP Compass

Brújula estilo Elden Ring para [Dusklight](https://github.com/TwilitRealm/dusklight) (Twilight Princess).

## Funciones
- Barra de brújula arriba de la pantalla con puntos cardinales (N, NE, E, SE, S, SO, O, NO)
- Marcas propias (diamante blanco) en tu posición actual, guardadas por partida
- Pilar de luz sobre cada marca, con aura y partículas: blanco-azulado de lejos, dorado
  (como el aura de Majora's Mask) cuando estás lo bastante cerca para quitarla
- Atajo de control para poner/quitar marcas (por defecto L + cruceta abajo)
- Cofres sin abrir (cuadro café/dorado), opcionalmente solo con la brújula del dungeon
- Golden bugs, Poes, piezas de corazón sueltas y Howling Stones sin aprender (de lo que
  está cargado alrededor de Link)
- Flechita arriba/abajo si un ícono está en otro piso; íconos más grandes al acercarte
- Puntos importantes (blancos con borde negro): pueblos, Castle Town, portales de Midna
  desbloqueados y entradas de dungeons/tiendas/Hyrule Castle aprendidas al visitarlas
- Marcas de otras zonas desde cualquier parte del overworld (las de dungeons/interiores
  apuntan a su entrada)
- Objetivo actual de la historia (círculo burgundy), oculto en dungeons y en randomizer
- Modo randomizer: items sueltos como chequeo genérico
- Nombre del lugar abajo o arriba a la izquierda (tabla de nombres de Dusklight, CC0)
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
