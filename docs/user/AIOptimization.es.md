# Asistente de ajustes con IA

Kaichii puede pedir a OpenAI o Anthropic una propuesta de ajustes para tu equipo. Necesitas una
clave de API propia del proveedor elegido; el servicio puede cobrarte por cada consulta. La función
es opcional: los perfiles de rendimiento y **Optimizar para este juego** siguen funcionando sin IA.

El asistente es propio de Kaichii: Lichii, Eden y yuzu no lo tienen. Además de ajustar la
configuración, puede [explicar el último problema](#explicar-el-ultimo-problema) a partir del
registro de Kaichii y [redactar un informe de compatibilidad](#redactar-un-informe-de-compatibilidad)
con tus propias palabras.

## Cómo usarlo

- **Para todos los juegos:** abre *Emulación > Pedir a la IA que ajuste este equipo*. Cierra el juego
  antes. La propuesta modifica los ajustes globales y se prueba tras reiniciar Kaichii.
- **Para un juego:** haz clic derecho en él y elige *Pedir a la IA que ajuste este juego*. También
  aparece en *Emulación* mientras juegas. Puedes consultar con el juego cerrado. Si está abierto,
  Kaichii añade a la consulta los FPS y el uso de memoria recientes, cuando hay datos. Para aplicar
  los cambios, te preguntará si quieres detener el juego; se prueban al iniciarlo de nuevo.

Para conseguir una clave, sigue el enlace que hay bajo el campo de la clave: abre la página de
claves del proveedor ([OpenAI](https://platform.openai.com/api-keys),
[Anthropic](https://platform.claude.com/settings/keys)). Elige el proveedor y el modelo, escribe tu clave, describe el problema y pulsa **Pedir una
propuesta**. La lista de modelos muestra los principales de cada proveedor, primero el recomendado;
puedes escribir otro nombre o pulsar **Cargar del proveedor** para ver los modelos que tu clave puede
usar. Kaichii recuerda el último proveedor y modelo que usaste.

Para no escribir la clave cada vez, marca **Recordar esta clave en el almacén de credenciales del
sistema**. Cuando una consulta sale bien, la guarda el sistema operativo: el Llavero en macOS, el
Administrador de credenciales en Windows y el Secret Service (GNOME Keyring, KWallet) en Linux, a
través de libsecret. Nunca se escribe en los archivos de configuración de Kaichii, y Kaichii solo la
envía a tu proveedor. **Olvidar la clave guardada** la borra, y también una consulta correcta con la
casilla desmarcada. Si no hay almacén de credenciales (Linux sin libsecret), la casilla aparece
desactivada y la clave solo permanece en memoria mientras la ventana está abierta.

La propuesta enseña cada cambio, su motivo y el valor anterior. Desmarca lo que no quieras aplicar.
Kaichii solo acepta opciones y valores de su catálogo, y guarda los ajustes anteriores. Puedes volver
a abrir el asistente y pulsar **Deshacer el último cambio de IA**. Si después modificas manualmente
alguno de esos ajustes, el asistente no lo sobrescribe al deshacer.

### Medir el resultado

Los ajustes cambian al iniciar el juego, así que el asistente mide antes y después de reiniciarlo.
Si el juego está abierto cuando aplicas un cambio, Kaichii guarda los FPS, el 1 % bajo y los tirones
de los últimos 10 segundos antes de detenerlo. Vuelve a iniciar el juego, juega unos segundos en la
misma escena, abre de nuevo el asistente y pulsa **Medir ahora (últimos 10 s)**. Verás las dos
mediciones y un veredicto: mejor, peor o sin cambio claro (unos pocos puntos arriba o abajo son la
diferencia normal entre dos pasadas por la misma escena). Pulsa **Mantener estos ajustes** si el
juego también se ve bien, o **Deshacer el último cambio de IA** si no. La siguiente consulta incluye
las mediciones, así que la IA sabe si su último cambio ayudó.

Compara en el mismo lugar: una escena distinta o la primera compilación de shaders alteran los FPS.
La IA puede explicar síntomas y elegir ajustes; no repara fallos de emulación. Si faltan texturas
incluso con ajustes apropiados, consulta [Solución de problemas](Troubleshooting.es.md).

## Explicar el último problema

*Ayuda > Explicar el último problema* lee el registro de Kaichii en busca de las causas habituales de
un juego que no arranca, se cierra o muestra algo mal: claves que faltan o son antiguas, un archivo
de juego dañado, falta de firmware, Vulkan o el controlador gráfico, la memoria de vídeo, shaders que
Kaichii aún no traduce y cierres del emulador. Esta parte no usa IA ni envía nada; cada hallazgo
enlaza con la página de esta guía que lo explica. Elige **La sesión anterior** si Kaichii se
reinició después del problema.

Para saber más, cuenta qué ha pasado (opcional) y pulsa **Pedir a la IA que lo explique**. La IA
responde en tu idioma, con lo que probablemente ha fallado y qué probar. Solo se envían las líneas del
registro sobre el problema y la versión, sin tu nombre de usuario en las rutas; marca **Mostrar las
líneas del registro que se enviarían** para leerlas antes. Si la respuesta apunta a un fallo de
Kaichii, **Preparar un informe de error en GitHub** abre el formulario de errores con un título, una
descripción y esas líneas: revísalo, adjunta `eden_log.txt` y publícalo con tu cuenta.

## Redactar un informe de compatibilidad

En un [informe de compatibilidad](CompatibilityReports.es.md), pulsa **Redactar con IA…**, cuenta
cómo ha ido el juego con tus palabras y en cualquier idioma, y pulsa **Redactar el informe**. La IA
escribe el resumen, el progreso, los detalles y los pasos en inglés, solo con lo que has dicho; tú
sigues eligiendo la compatibilidad y la fluidez, y lo revisas todo antes de abrirlo en GitHub. Se
envía: tus palabras y lo que el informe ya incluye (el juego, la versión, tu tarjeta gráfica, el
sistema operativo y los ajustes de inicio).

## Datos y alcance

Una consulta de ajustes incluye tu descripción, el nombre e identificador del juego, sistema operativo, GPU,
VRAM, hilos de CPU, RAM, ajustes pertinentes, el último cambio hecho por el asistente y, si el juego
está en marcha, las métricas recientes disponibles, además del resultado medido del último cambio.
No se
envían juegos, partidas, claves de la consola, capturas ni el registro completo. Con el modo
desarrollador activado (en la configuración), la ventana muestra además el diagnóstico exacto, en
**Datos enviados con esta consulta**, antes de enviarlo. El resultado se valida localmente; una respuesta inválida
no cambia la configuración.
