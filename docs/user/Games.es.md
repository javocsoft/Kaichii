# Juegos, claves y firmware

Kaichii solo ejecuta juegos que hayas volcado de tu propia consola. No descarga ni incluye ninguno.

## Qué necesita cada tipo de archivo

| Archivo | Qué es | Necesita |
|---|---|---|
| `.nup` | Un paquete de juego de Switch 2 | Nada más |
| `.nsp`, `.nsz`, `.xci`, `.xcz` | Juegos de Switch, instalables o volcados de cartucho | Las claves de descifrado de tu consola; algunos juegos también su firmware |
| `.nro`, `.nso` | Homebrew y ejecutables | Nada más |

## Claves

Las claves salen de tu propia consola, en un archivo `prod.keys`. Instálalas con *Herramientas >
Instalar claves de descifrado* (o desde la primera página del asistente) y elige el archivo. Kaichii
lo copia a su carpeta `keys`.

## Firmware

Algunos juegos y los applets del sistema necesitan el firmware de tu consola. Instálalo con
*Herramientas > Instalar firmware > Desde una carpeta* o *Desde un archivo ZIP*, según lo hayas
volcado.

## Actualizaciones y DLC

Instálalos con *Archivo > Instalar archivos en NAND*. En el menú del clic derecho de un juego,
*Configurar juego > Complementos* activa o desactiva cada uno. Los detalles están en
[Updates and DLC](InstallingUpdatesDLC.md) (en inglés).

Siguiente: [Ajustes para tu equipo](Performance.es.md).
