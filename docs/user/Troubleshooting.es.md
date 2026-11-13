# Solución de problemas

## El registro

Kaichii anota lo que pasa en un registro. *Archivo > Abrir carpetas de Kaichii > Carpeta de
registros* abre su carpeta: `eden_log.txt` es la sesión actual o la última, y `eden_log.txt.old` la
anterior. Su primera línea indica la versión. Adjúntalo cuando informes de un problema, después de
revisar que no lleve datos personales.

## Kaichii no arranca

- **Linux:** haz ejecutable el AppImage (`chmod +x`); si menciona FUSE, consulta
  [Instalar Kaichii](Install.es.md#linux).
- **Windows:** extrae la carpeta entera, no solo `kaichii.exe`. Si falta `VCRUNTIME140.dll` o
  `MSVCP140.dll`, hace falta el Visual C++ Redistributable (consulta
  [Instalar Kaichii](Install.es.md#windows)).
- **macOS:** la app no está notarizada: la primera vez haz clic derecho sobre ella y elige *Abrir*.
- **Cualquier sistema:** actualiza el controlador gráfico. Kaichii dibuja con Vulkan, y un
  controlador antiguo es el motivo más habitual de que un juego no arranque o se vea en negro.

## El juego va lento

- Usa el ajuste predefinido que recomienda Kaichii, o *Probar este equipo*, en *Configurar >
  Rendimiento*.
- Prueba **Optimizar para este juego** desde el menú del clic derecho del juego.
- En un portátil con dos tarjetas gráficas, elige la dedicada en *Configurar > Imagen*,
  **Dispositivo**.
- Consulta [Ajustes para tu equipo](Performance.es.md).

## El juego da tirones

La primera vez que aparece cada efecto se construyen sus shaders, y eso puede dar tirones. Van
desapareciendo al jugar y las siguientes visitas son más fluidas (consulta
[Tirones por shaders](Performance.es.md#tirones-por-shaders)).

## Las ventanas o los diálogos se ven mal en Linux

En escritorios Wayland, Kaichii dibuja sus ventanas a través de X11 (XWayland) por defecto, para que
los diálogos quepan y se queden en su sitio. **Forzar X11 como motor gráfico**, en *Configurar >
Interfaz* (con *Mostrar opciones avanzadas*), lo activa o desactiva; se aplica al reiniciar Kaichii.

## Informar de un error

1. Actualiza a la [última versión](https://github.com/javocsoft/Kaichii/releases): puede que ya esté
   arreglado.
2. Busca el mismo problema en las [incidencias abiertas](https://github.com/javocsoft/Kaichii/issues).
3. [Abre una incidencia](https://github.com/javocsoft/Kaichii/issues/new/choose) con el formulario
   que corresponda (juego, cierre inesperado, gráficos o interfaz) e incluye:
   - la versión (la primera línea del registro), el juego y su versión;
   - tu procesador, tarjeta gráfica, versión del controlador y sistema operativo;
   - qué hiciste, qué pasó y qué esperabas, con una captura si ayuda;
   - el registro de esa sesión.

No subas nunca juegos, claves, firmware ni datos de tu cuenta.

Para preguntas y consejos, o si no tienes claro que algo sea un error, pregunta en
[Discussions](https://github.com/javocsoft/Kaichii/discussions).
