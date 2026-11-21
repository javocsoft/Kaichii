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

## Cómo muestra la lista tus juegos

*Ver > Modo de la lista de juegos* elige una de cinco vistas: *Vista en árbol* (una tabla con
detalles), *Vista en cuadrícula*, *Vista de carrusel*, *Vista en carátulas* (los iconos de los
juegos, grandes, como en una estantería) y *Vista en cajas* (la caja de cada juego, como en una
tienda).

*Ver > Ordenar juegos por* ordena todas las vistas menos la de árbol, y recuerda lo que elijas:

- *Nombre*: como se leen, sin distinguir mayúsculas ni acentos y con los números por su valor
  (*Mario 2* antes que *Mario 10*).
- *Fecha en que se añadió*: cuándo llegó el archivo del juego a este equipo (copiado, descargado o
  convertido).
- *Tiempo de juego*: cuánto has jugado a cada uno; se actualiza al cerrar un juego.

*Ascendente* y *Descendente* dan la vuelta a cualquiera de ellos. Al elegir un orden empieza en el
sentido más útil: los nombres desde la A, y los más recientes y los más jugados primero. Los juegos
empatados, como los que nunca has jugado, siguen por nombre. La vista en árbol ordena por la
columna en la que hagas clic.

Haz clic derecho en una parte vacía de la lista para tener las mismas opciones: el modo de la lista,
el orden y las secciones de Switch 2. En las dos vistas de carátulas, *Ver >
Separar los juegos de Switch 2* muestra primero los juegos de Switch 2, bajo su propio título, y
después los de Switch bajo el suyo. Un juego cuenta como de Switch 2 por su id (los de Switch 2
empiezan por `04`), así que un juego de Switch con la mejora Switch 2 Edition se queda con los de
Switch.

La vista en cajas muestra la imagen de la caja que elijas. Abre las propiedades del juego (clic
derecho, *Configurar juego*), elige la pestaña *Caja* encima de los datos del juego, después
*Elegir imagen de la caja...* y *Aceptar*. *Quitar* vuelve al icono. Un juego sin imagen de caja
muestra su icono sobre una caja, para que la estantería no tenga huecos. Funciona con cualquier
formato de juego y nunca modifica el archivo del juego.

Kaichii guarda las cajas en la carpeta `box_art` de su carpeta de datos (mira
[dónde se guardan tus datos](Install.es.md#donde-se-guardan-tus-datos)). Para añadir muchas a la vez, copia ahí las
imágenes con el id del juego como nombre, tal como lo muestra *Configurar juego*, en mayúsculas:
`0400486024DEC000.jpg` para Ocarina of Time. Valen PNG, JPEG y WebP, mejor con forma de unos 105
por 170; las de otra forma se recortan para llenar la caja.

### Un icono propio para un juego

La pestaña *Icono* de las propiedades de un juego cambia el icono que muestran todas las vistas. En
NSP, XCI y los demás formatos, *Subir icono...* toma una imagen tuya, recortada a cuadrado, y
*Restaurar icono del juego* vuelve al del juego; el archivo del juego nunca se modifica, y el icono
se guarda en la carpeta `custom_icons` de la carpeta de datos de Kaichii, con el mismo nombre que
las cajas. Los juegos NUP siguen igual: su *Subir icono...* y *Detectar icono* escriben el icono
dentro del propio archivo `.nup`.

Siguiente: [Ajustes para tu equipo](Performance.es.md).
