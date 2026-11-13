# Ajustes para tu equipo

Casi todo lo que hace que un juego funcione bien en un equipo modesto depende de unos pocos ajustes.
Kaichii los elige por ti: solo tienes que revisarlos cuando un juego pide más de lo que tu equipo
tiene.

## Ajustes predefinidos

*Configurar > Rendimiento* muestra lo que Kaichii ha encontrado en el equipo (tarjeta gráfica,
memoria de vídeo, hilos del procesador y memoria) y cuatro ajustes predefinidos. El adecuado para
este equipo aparece como recomendado.

| Ajuste | Resolución | Para |
|---|---|---|
| Rendimiento | La mitad, reescalada con FSR | Gráficos integrados, portátiles, tarjetas de 4 GB o menos |
| Equilibrado | Tres cuartos, reescalada con FSR | Tarjetas de 6 a 8 GB |
| Calidad | Nativa | Tarjetas de 10 a 12 GB |
| Ultra | Una vez y media la nativa | Tarjetas de 16 GB o más |

Todos los ajustes predefinidos usan además la precisión de GPU rápida y los shaders asíncronos,
ajustan la memoria de texturas a la tarjeta y descodifican las texturas ASTC en el procesador cuando
tiene 8 hilos o más: la mayoría de las tarjetas gráficas de PC no leen ASTC, el formato de los juegos
de Switch, y descodificarlo en la tarjeta compite con el juego. En procesadores con al menos seis
núcleos de rendimiento, también dejan los hilos de la GPU en núcleos de rendimiento propios. En
Ocarina of Time, Rendimiento además omite la pasada de nitidez del juego.

- **Usar este ajuste predefinido** rellena los ajustes. No se guarda nada hasta que pulsas
  *Aceptar*.
- **Probar este equipo** mide la velocidad de la tarjeta gráfica, la memoria de vídeo libre en ese
  momento y la velocidad del procesador en unos diez segundos, y recomienda un ajuste a partir de
  ello. Cierra antes cualquier juego.
- En *Configurar juego > Rendimiento* de un juego, la misma página ajusta solo ese juego.

## Optimizar para este juego

Haz clic derecho en un juego y elige **Optimizar para este juego** (mientras juegas: *Emulación >
Optimizar para este juego*, o Ctrl+Mayús+O). Kaichii mide el juego mientras funciona, prueba la
generación de fotogramas durante unos segundos y propone cambios; no cambia nada hasta que aceptas.
Ponte en un sitio exigente, como un exterior con la cámara quieta, y no pauses ni abras menús
mientras mide.

## Con una tarjeta de 4 GB

- Empieza con el ajuste **Rendimiento**.
- Si las escenas cargadas se ralentizan mucho o desaparecen imágenes, se está acabando la memoria de
  vídeo: baja la *Resolución* (*Configurar > Imagen*) o pon **Modo de uso de la VRAM** en
  *Conservador*.
- Deja **Modo descodificación ASTC** en *CPU* o *CPU asíncrona*, salvo que el procesador sea lento.

## Tirones por shaders

La primera vez que aparece un efecto, sus shaders tienen que construirse para tu tarjeta gráfica, y
el juego puede dar un tirón. Kaichii construye los shaders nuevos de Switch 2 en segundo plano
mientras el juego sigue, los guarda para la próxima vez y prepara los de sesiones anteriores antes de
que el juego los necesite. Los tirones son peores en la primera partida y van desapareciendo al
jugar. En Linux con una tarjeta NVIDIA, Kaichii da a la caché de shaders del controlador espacio para
juegos grandes.

## Medido

The Legend of Zelda: Ocarina of Time en un portátil con una RTX 3050 (4 GB), en la casa de Link:

| Ajustes | fps |
|---|---|
| Resolución nativa, el suavizado del juego, ASTC en la GPU (los valores antiguos) | 14 |
| Ajuste Rendimiento | 40 |
| Ajuste Equilibrado | 32 |

Un juego en un equipo: tus resultados dependen del juego, del hardware y del controlador. La
[lista de compatibilidad](Compatibility.es.md) muestra lo que ven otros jugadores.

Todas las opciones gráficas en detalle, en [Graphics settings in detail](Graphics.md) (en inglés).
