# The Legend of Zelda: Ocarina of Time

La edición de Switch 2 (ID de programa `0400486024DEC000`) es el juego con el que se ajusta y se
mide Kaichii. Viene como paquete NUP, así que no necesita claves ni firmware.

## Ajustes recomendados

- **Con una tarjeta de 4 GB, el ajuste Rendimiento.** Dibuja a la mitad de resolución, reescala con
  FSR, descodifica las texturas en el procesador y desactiva la pasada de nitidez del juego.
- **Antialiasing de Ocarina of Time**, en *Configurar juego > Imagen* (solo aparece en los ajustes de
  este juego):
  - *Juego* mantiene la red de suavizado propia del juego, que da mucha nitidez.
  - *Sin enfoque* muestra la imagen tal como la dibuja el juego, sin esa red: más rápido, y combina
    bien con una resolución más alta.

  Se aplica la próxima vez que arranca el juego, y no hace nada mientras NVIDIA DLSS está activo.

## Qué esperar

Medido en un portátil con una RTX 3050 (4 GB), en la casa de Link:

| Ajustes | fps |
|---|---|
| Resolución nativa, el suavizado del juego, texturas descodificadas en la tarjeta gráfica | 14 |
| Resolución nativa, *Sin enfoque*, texturas descodificadas en el procesador | 31 |
| Ajuste Equilibrado | 32 |
| Ajuste Rendimiento | 40 |

En exteriores marca el ritmo el hilo principal del propio juego en el procesador, así que bajar los
ajustes gráficos ayuda menos que en interiores, y un procesador más rápido ayuda más. La primera
visita a cada zona puede dar tirones mientras se construyen sus shaders; las siguientes van más
fluidas (consulta [Tirones por shaders](Performance.es.md#tirones-por-shaders)).

## Informes

La [lista de compatibilidad](Compatibility.es.md) muestra cómo funciona el juego para otros
jugadores. Después de jugar, [envía tu propio informe](CompatibilityReports.es.md): dice qué
hardware y qué ajustes funcionan.
